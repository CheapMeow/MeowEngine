// DAP smoke test for MeowEngine's embedded quickjs-ng debug server.
//
// Launches the engine with MEOW_DEBUG=1, speaks the Debug Adapter Protocol
// over TCP, sets a breakpoint in scripts/init_scene.js and verifies that the
// engine actually stops there and can report a stack trace and variables.
//
// Usage: node tools/dap_smoke_test.js [path-to-exe]

const net = require('net');
const path = require('path');
const { spawn } = require('child_process');

const ENGINE_ROOT = path.resolve(__dirname, '..');
const EXE = process.argv[2] ||
    path.join(ENGINE_ROOT, 'build', 'src', 'meow_editor', 'Debug', 'MeowEditor.exe');
const SCRIPT = path.join(ENGINE_ROOT, 'scripts', 'init_scene.js').replace(/\\/g, '/');
const BREAK_LINE = Number(process.env.MEOW_BP_LINE || 22);   // default: `let camObj = M.createObject();`
const PORT = 9229;
const HOST = '127.0.0.1';

const results = [];
function check(name, ok, detail) {
    results.push({ name, ok, detail });
    console.log(`${ok ? 'PASS' : 'FAIL'}  ${name}${detail ? '  -- ' + detail : ''}`);
}

// ---- DAP framing ----

class DapConnection {
    constructor(socket) {
        this.socket = socket;
        this.seq = 1;
        this.buffer = Buffer.alloc(0);
        this.pendingRequests = new Map();
        this.eventWaiters = [];
        this.events = [];
        socket.on('data', (chunk) => this._onData(chunk));
    }

    _onData(chunk) {
        this.buffer = Buffer.concat([this.buffer, chunk]);
        for (;;) {
            const headerEnd = this.buffer.indexOf('\r\n\r\n');
            if (headerEnd < 0) return;
            const header = this.buffer.subarray(0, headerEnd).toString('ascii');
            const match = /Content-Length:\s*(\d+)/i.exec(header);
            if (!match) { this.buffer = this.buffer.subarray(headerEnd + 4); continue; }
            const length = parseInt(match[1], 10);
            const bodyStart = headerEnd + 4;
            if (this.buffer.length < bodyStart + length) return;
            const body = this.buffer.subarray(bodyStart, bodyStart + length).toString('utf8');
            this.buffer = this.buffer.subarray(bodyStart + length);
            let msg;
            try { msg = JSON.parse(body); } catch { continue; }
            this._dispatch(msg);
        }
    }

    _dispatch(msg) {
        if (msg.type === 'response') {
            const resolver = this.pendingRequests.get(msg.request_seq);
            if (resolver) { this.pendingRequests.delete(msg.request_seq); resolver(msg); }
        } else if (msg.type === 'event') {
            this.events.push(msg);
            this.eventWaiters = this.eventWaiters.filter((waiter) => {
                if (waiter.name !== msg.event) return true;
                waiter.resolve(msg);
                return false;
            });
        }
    }

    send(command, args, timeoutMs = 15000) {
        const seq = this.seq++;
        const payload = JSON.stringify({ seq, type: 'request', command, arguments: args || {} });
        this.socket.write(`Content-Length: ${Buffer.byteLength(payload, 'utf8')}\r\n\r\n${payload}`);
        return new Promise((resolve, reject) => {
            const timer = setTimeout(
                () => reject(new Error(`timeout waiting for response to "${command}"`)), timeoutMs);
            this.pendingRequests.set(seq, (msg) => { clearTimeout(timer); resolve(msg); });
        });
    }

    waitForEvent(name, timeoutMs = 20000) {
        const existing = this.events.find((e) => e.event === name);
        if (existing) return Promise.resolve(existing);
        return new Promise((resolve, reject) => {
            const timer = setTimeout(
                () => reject(new Error(`timeout waiting for event "${name}"`)), timeoutMs);
            this.eventWaiters.push({ name, resolve: (m) => { clearTimeout(timer); resolve(m); } });
        });
    }
}

// ---- helpers ----

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

async function waitForPort(host, port, timeoutMs) {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
        const ok = await new Promise((resolve) => {
            const sock = net.connect({ host, port });
            sock.once('connect', () => { sock.destroy(); resolve(true); });
            sock.once('error', () => resolve(false));
        });
        // The probe consumed the pending accept(); give the server a moment to
        // loop back around before the real client connects.
        if (ok) return true;
        await sleep(200);
    }
    return false;
}

// ---- main ----

(async () => {
    console.log(`engine : ${EXE}`);
    console.log(`script : ${SCRIPT}:${BREAK_LINE}\n`);

    const child = spawn(EXE, [], {
        cwd: ENGINE_ROOT,
        env: { ...process.env, MEOW_DEBUG: '1', MEOW_DEBUG_PORT: String(PORT) },
        stdio: ['ignore', 'pipe', 'pipe'],
    });

    let engineOutput = '';
    child.stdout.on('data', (d) => { engineOutput += d.toString(); });
    child.stderr.on('data', (d) => { engineOutput += d.toString(); });

    const cleanup = () => { try { child.kill(); } catch { /* already gone */ } };
    process.on('exit', cleanup);

    try {
        // 1. The engine must announce and open the debug port.
        const listening = await new Promise(async (resolve) => {
            const deadline = Date.now() + 30000;
            while (Date.now() < deadline) {
                if (engineOutput.includes('listening on')) return resolve(true);
                if (child.exitCode !== null) return resolve(false);
                await sleep(200);
            }
            resolve(false);
        });
        check('engine announces DAP server', listening,
              listening ? engineOutput.trim().split('\n').pop() : 'no "listening on" line');
        if (!listening) throw new Error('engine never started the debug server');

        // 2. Connect and run the DAP handshake.
        const socket = await new Promise((resolve, reject) => {
            const s = net.connect({ host: HOST, port: PORT });
            s.once('connect', () => resolve(s));
            s.once('error', reject);
        });
        check('TCP connect to debug port', true, `${HOST}:${PORT}`);

        const dap = new DapConnection(socket);

        const init = await dap.send('initialize', {
            adapterID: 'meow-smoke', clientID: 'smoke', linesStartAt1: true, columnsStartAt1: true,
        });
        check('initialize', init.success === true,
              `supportsConfigurationDoneRequest=${init.body && init.body.supportsConfigurationDoneRequest}`);

        const attach = await dap.send('attach', { stopOnEntry: false });
        check('attach', attach.success === true);

        await dap.waitForEvent('initialized', 10000);
        check('initialized event', true);

        // 3. Set a breakpoint before releasing the engine.
        const bps = await dap.send('setBreakpoints', {
            source: { path: SCRIPT },
            breakpoints: [{ line: BREAK_LINE }],
        });
        const verified = bps.success && bps.body && Array.isArray(bps.body.breakpoints) &&
                         bps.body.breakpoints.length === 1;
        check('setBreakpoints accepted', verified, JSON.stringify(bps.body));

        const configDone = await dap.send('configurationDone', {});
        check('configurationDone', configDone.success === true);

        // 4. The engine now evaluates init_scene.js -- it must stop on our line.
        const stopped = await dap.waitForEvent('stopped', 30000);
        check('stopped event received', true,
              `reason=${stopped.body && stopped.body.reason}`);
        check('stopped due to breakpoint',
              stopped.body && stopped.body.reason === 'breakpoint',
              `reason=${stopped.body && stopped.body.reason}`);

        // 5. Inspect the paused state.
        const stack = await dap.send('stackTrace', { threadId: 1 });
        const frames = (stack.body && stack.body.stackFrames) || [];
        check('stackTrace returns frames', frames.length > 0,
              frames.length ? `top=${frames[0].name} @ line ${frames[0].line}` : 'no frames');
        check('stopped on the requested line',
              frames.length > 0 && frames[0].line === BREAK_LINE,
              frames.length ? `line=${frames[0].line}, want ${BREAK_LINE}` : 'no frames');

        // Hammer the inspection requests the same way the standalone probe
        // does, recording latency so a stall is easy to spot.
        const sequence = ['threads', 'stackTrace', 'scopes', 'variables',
                          'threads', 'stackTrace', 'scopes', 'variables'];
        let frameId = frames.length ? frames[0].id : 0;
        let localsRef = 10000;
        let answered = 0;

        for (const cmd of sequence) {
            let args = {};
            if (cmd === 'stackTrace') args = { threadId: 1 };
            else if (cmd === 'scopes') args = { frameId };
            else if (cmd === 'variables') args = { variablesReference: localsRef };

            const t0 = Date.now();
            const resp = await dap.send(cmd, args, 8000).catch(() => null);
            const dt = Date.now() - t0;

            if (!resp) {
                console.log(`    TIMEOUT ${cmd.padEnd(12)} ${dt}ms  (request #${answered + 1})`);
                break;
            }
            answered++;
            if (cmd === 'stackTrace' && resp.body && resp.body.stackFrames &&
                resp.body.stackFrames.length) {
                frameId = resp.body.stackFrames[0].id;
            }
            if (cmd === 'scopes' && resp.body && resp.body.scopes && resp.body.scopes.length) {
                localsRef = resp.body.scopes[0].variablesReference;
            }
            let extra = '';
            if (cmd === 'variables' && resp.body && resp.body.variables) {
                extra = ` (${resp.body.variables.length} vars)`;
            }
            console.log(`    ok      ${cmd.padEnd(12)} ${dt}ms${extra}`);
        }

        check('all paused-state inspection requests answered',
              answered === sequence.length, `${answered}/${sequence.length}`);
        check('engine still alive while paused', child.exitCode === null,
              `exitCode=${child.exitCode}`);

        // 6. Stepping must work too.
        await dap.send('next', { threadId: 1 });
        const steppedEvent = await dap.waitForEvent('stopped', 20000).catch(() => null);
        check('step over produces a stop', steppedEvent !== null,
              steppedEvent ? `reason=${steppedEvent.body && steppedEvent.body.reason}` : 'timed out');

        // 7. Resume and disconnect cleanly.
        await dap.send('continue', { threadId: 1 });
        check('continue accepted', true);

        await dap.send('disconnect', {}).catch(() => {});
        socket.end();
    } catch (err) {
        check('smoke test completed without errors', false, err.message);
    } finally {
        await sleep(500);
        cleanup();
    }

    const failed = results.filter((r) => !r.ok);
    console.log(`\n${results.length - failed.length}/${results.length} checks passed`);
    if (engineOutput.trim()) {
        console.log('\n--- engine output ---');
        console.log(engineOutput.trim().split('\n').slice(-25).join('\n'));
    }
    process.exit(failed.length === 0 ? 0 : 1);
})();
