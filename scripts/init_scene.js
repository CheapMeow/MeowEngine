// Scene initialization — replaces the hardcoded setup in EditorWindow constructor.

// ---- helpers ----

function eulerToQuat(x, y, z) {
    let cx = Math.cos(x * 0.5), sx = Math.sin(x * 0.5);
    let cy = Math.cos(y * 0.5), sy = Math.sin(y * 0.5);
    let cz = Math.cos(z * 0.5), sz = Math.sin(z * 0.5);
    return [
        cx * cy * cz + sx * sy * sz,  // w
        sx * cy * cz - cx * sy * sz,  // x
        cx * sy * cz + sx * cy * sz,  // y
        cx * cy * sz - sx * sy * cz   // z
    ];
}

let M = MeowNative;
let vAttrs = M.getDefaultVertexAttributes();
let matID  = M.getDefaultMaterialID();

// ---- Camera ----
let camObj = M.createObject();
M.setName(camObj.ptr, "Camera");
M.setMainCameraID(camObj.uuid);

let camT = M.addComponent(camObj.ptr, "Transform3DComponent");
let camTp = new Transform3DComponent(camT.ptr);
camTp.position = [0.0, 10.0, -6.0];
camTp.rotation = eulerToQuat(-100.0, 0.0, 0.0);

let cam = M.addComponent(camObj.ptr, "Camera3DComponent");
let camp = new Camera3DComponent(cam.ptr);
camp.camera_mode = "Free";

// ---- Directional Light ----
let lightObj = M.createObject();
M.setName(lightObj.ptr, "DirectionalLight");

let lightT = M.addComponent(lightObj.ptr, "Transform3DComponent");
let lightTp = new Transform3DComponent(lightT.ptr);
lightTp.position = [0.0, 30.0, -50.0];
lightTp.rotation = eulerToQuat(-100.0, 0.0, 0.0);

M.addComponent(lightObj.ptr, "DirectionalLightComponent");

// ---- Cube ----
let cubeObj = M.createObject();
M.setName(cubeObj.ptr, "Cube");

let cubeT = M.addComponent(cubeObj.ptr, "Transform3DComponent");
let cubeTp = new Transform3DComponent(cubeT.ptr);
cubeTp.position = [0.0, 0.0, 10.0];

let cubeMesh = M.createCubeMesh(vAttrs);
let cubeModel = M.createModel(cubeMesh.vertices, cubeMesh.indices, vAttrs);
let cubeComp = M.addComponent(cubeObj.ptr, "ModelComponent");
M.setModelComponent(cubeComp.ptr, cubeModel.ptr, matID);

// ---- Plane ----
let planeObj1 = M.createObject();
M.setName(planeObj1.ptr, "Plane");

let planeT1 = M.addComponent(planeObj1.ptr, "Transform3DComponent");
let planeTp1 = new Transform3DComponent(planeT1.ptr);
planeTp1.position = [0.0, -10.0, 10.0];
planeTp1.scale    = [20.0, 20.0, 20.0];

let planeMesh1 = M.createPlaneMesh(vAttrs);
let planeModel1 = M.createModel(planeMesh1.vertices, planeMesh1.indices, vAttrs);
let planeComp1 = M.addComponent(planeObj1.ptr, "ModelComponent");
M.setModelComponent(planeComp1.ptr, planeModel1.ptr, matID);

// ---- Plane for test ----
let planeObj2 = M.createObject();
M.setName(planeObj2.ptr, "Plane for test");

let planeT2 = M.addComponent(planeObj2.ptr, "Transform3DComponent");
let planeTp2 = new Transform3DComponent(planeT2.ptr);
planeTp2.position = [0.0, 0.0, 0.0];
planeTp2.scale    = [20.0, 20.0, 20.0];

let planeMesh2 = M.createPlaneMesh(vAttrs);
let planeModel2 = M.createModel(planeMesh2.vertices, planeMesh2.indices, vAttrs);
let planeComp2 = M.addComponent(planeObj2.ptr, "ModelComponent");
M.setModelComponent(planeComp2.ptr, planeModel2.ptr, matID);

// ---- invoke reflective test methods ----
let cubeMC = new ModelComponent(cubeComp.ptr);
cubeMC.foo1();
cubeMC.foo2();
