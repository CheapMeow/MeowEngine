#include "code_generator.h"

#include "utils/code_gen_utils.h"

#include <iomanip>

namespace Meow
{
    void CodeGenerator::Begin(const std::string& src_path, const std::string& output_path)
    {
        if (is_recording)
        {
            std::cerr << "Parser is already recording." << std::endl;
            return;
        }

        is_recording = true;

        this->src_path    = fs::path(src_path);
        this->output_path = fs::path(output_path);
    }

    void CodeGenerator::Generate(const std::vector<std::string>&      include_relative_paths,
                                 const std::vector<ClassParseResult>& class_results,
                                 const std::vector<EnumParseResult>&  enum_results)
    {
        std::string tmpl = CodeGenUtils::read_template("register_all.cpp.tmpl");
        if (tmpl.empty())
            return;

        // build includes
        std::stringstream includes;
        for (const auto& p : include_relative_paths)
            includes << "#include \"" << p << "\"\n";

        // build class registrations
        std::stringstream registrations;
        bool              is_first = true;

        for (const auto& cls : class_results)
        {
            if (!is_first)
                registrations << std::endl;
            is_first = false;

            registrations << "\t\treflect::AddClass<" << cls.class_name << ">(" << std::quoted(cls.class_name) << ")";

            for (const auto& f : cls.field_results)
            {
                if (f.is_array)
                    registrations << "\n\t\t\t.AddArray(" << std::quoted(f.field_name) << ", "
                                  << std::quoted(f.field_type_name) << ", " << std::quoted(f.inner_type_name) << ", &"
                                  << cls.class_name << "::" << f.field_name << ")";
                else
                    registrations << "\n\t\t\t.AddField(" << std::quoted(f.field_name) << ", "
                                  << std::quoted(f.field_type_name) << ", &" << cls.class_name << "::" << f.field_name
                                  << ")";
            }

            for (const auto& m : cls.method_results)
                registrations << "\n\t\t\t.AddMethod(" << std::quoted(m.method_name) << ", &" << cls.class_name
                              << "::" << m.method_name << ")";

            registrations << ";\n";
        }

        // build enum converters
        std::stringstream enums;
        for (const auto& e : enum_results)
        {
            GenerateEnumReflHeaderFile(e);

            std::stringstream gen1, gen2;

            gen1 << "\t" << e.enum_name << " to_enum(const std::string& str)" << std::endl;
            gen1 << "\t{";

            gen2 << "\tconst std::string to_string(" << e.enum_name << " enum_val)" << std::endl;
            gen2 << "\t{" << std::endl;
            gen2 << "\t\tswitch (enum_val)" << std::endl;
            gen2 << "\t\t{";

            for (const auto& elem : e.enum_element_names)
            {
                gen1 << "\n\t\tif (str == " << std::quoted(elem) << ")";
                gen1 << "\n\t\t\treturn " << e.enum_name << "::" << elem << ";";

                gen2 << "\n\t\t\tcase " << e.enum_name << "::" << elem << ":";
                gen2 << "\n\t\t\t\treturn " << std::quoted(elem) << ";";
            }

            gen1 << std::endl;
            gen1 << std::endl;
            gen1 << "\t\treturn " << e.enum_name << "::None;" << std::endl;
            gen1 << "\t}" << std::endl;
            gen1 << std::endl;

            gen2 << "\n\t\t\tdefault:" << std::endl;
            gen2 << "\t\t\t\treturn \"Unknown\";" << std::endl;
            gen2 << "\t\t}" << std::endl;
            gen2 << "\t}" << std::endl;
            gen2 << std::endl;

            enums << gen1.str() << gen2.str();
        }

        CodeGenUtils::replace_all_inplace(tmpl, "{{INCLUDES}}", includes.str());
        CodeGenUtils::replace_all_inplace(tmpl, "{{CLASS_REGISTRATIONS}}", registrations.str());
        CodeGenUtils::replace_all_inplace(tmpl, "{{ENUM_CONVERTERS}}", enums.str());

        std::ofstream out(output_path.string() + "/register_all.cpp");
        out << tmpl;
        out.close();

        std::cout << "[CodeGenerator] Generated register_all.cpp" << std::endl;
    }

    void CodeGenerator::GenerateJSBindingCpp(const std::vector<std::string>&      include_relative_paths,
                                             const std::vector<ClassParseResult>& class_results)
    {
        std::string tmpl = CodeGenUtils::read_template("register_js_binding.cpp.tmpl");
        if (tmpl.empty())
            return;

        // collect classes with JS bindings
        std::vector<const ClassParseResult*> js_classes;
        for (const auto& cls : class_results)
        {
            bool has = false;
            for (const auto& f : cls.field_results)
                if (f.has_js_binding)
                {
                    has = true;
                    break;
                }
            if (!has)
                for (const auto& m : cls.method_results)
                    if (m.has_js_binding)
                    {
                        has = true;
                        break;
                    }
            if (has)
                js_classes.push_back(&cls);
        }

        // build includes
        std::stringstream includes;
        for (const auto& p : include_relative_paths)
            includes << "#include \"" << p << "\"\n";

        // build functions and registrations
        std::stringstream functions;
        std::stringstream registrations;

        for (const auto* cls : js_classes)
        {
            for (const auto& f : cls->field_results)
            {
                if (!f.has_js_binding)
                    continue;

                std::string cls_name = cls->class_name;
                std::string fld_name = f.field_name;
                std::string type     = f.field_type_name;

                // getter
                functions << "// --- " << cls_name << "::" << fld_name << " (" << type << ") ---\n";
                functions << "static JSValue js_get_" << cls_name << "_" << fld_name
                          << "(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)\n";
                functions << "{\n";
                functions << "    double ptr_val; JS_ToFloat64(ctx, &ptr_val, argv[0]);\n";
                functions << "    auto* obj = reinterpret_cast<" << cls_name << "*>(static_cast<uint64_t>(ptr_val));\n";
                functions << "    return JSConvert<" << type << ">::ToJS(ctx, obj->" << fld_name << ");\n";
                functions << "}\n\n";

                // setter
                functions << "static JSValue js_set_" << cls_name << "_" << fld_name
                          << "(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)\n";
                functions << "{\n";
                functions << "    double ptr_val; JS_ToFloat64(ctx, &ptr_val, argv[0]);\n";
                functions << "    auto* obj = reinterpret_cast<" << cls_name << "*>(static_cast<uint64_t>(ptr_val));\n";
                functions << "    obj->" << fld_name << " = JSConvert<" << type << ">::FromJS(ctx, argv[1]);\n";
                functions << "    return JS_UNDEFINED;\n";
                functions << "}\n\n";

                // registration
                std::string get_name = "__get_" + cls_name + "_" + fld_name;
                std::string set_name = "__set_" + cls_name + "_" + fld_name;

                registrations << "    JS_DefinePropertyValueStr(ctx, global, \"" << get_name << "\",\n";
                registrations << "        JS_NewCFunction(ctx, js_get_" << cls_name << "_" << fld_name << ", \""
                              << get_name << "\", 1),\n";
                registrations << "        JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);\n\n";

                registrations << "    JS_DefinePropertyValueStr(ctx, global, \"" << set_name << "\",\n";
                registrations << "        JS_NewCFunction(ctx, js_set_" << cls_name << "_" << fld_name << ", \""
                              << set_name << "\", 2),\n";
                registrations << "        JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);\n\n";
            }

            for (const auto& m : cls->method_results)
            {
                if (!m.has_js_binding)
                    continue;

                std::string cls_name = cls->class_name;
                std::string mtd_name = m.method_name;

                functions << "// --- " << cls_name << "::" << mtd_name << "() ---\n";
                functions << "static JSValue js_call_" << cls_name << "_" << mtd_name
                          << "(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)\n";
                functions << "{\n";
                functions << "    double ptr_val; JS_ToFloat64(ctx, &ptr_val, argv[0]);\n";
                functions << "    auto* obj = reinterpret_cast<" << cls_name << "*>(static_cast<uint64_t>(ptr_val));\n";
                functions << "    obj->" << mtd_name << "();\n";
                functions << "    return JS_UNDEFINED;\n";
                functions << "}\n\n";

                std::string call_name = "__call_" + cls_name + "_" + mtd_name;

                registrations << "    JS_DefinePropertyValueStr(ctx, global, \"" << call_name << "\",\n";
                registrations << "        JS_NewCFunction(ctx, js_call_" << cls_name << "_" << mtd_name << ", \""
                              << call_name << "\", 1),\n";
                registrations << "        JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);\n\n";
            }
        }

        CodeGenUtils::replace_all_inplace(tmpl, "{{INCLUDES}}", includes.str());
        CodeGenUtils::replace_all_inplace(tmpl, "{{FUNCTIONS}}", functions.str());
        CodeGenUtils::replace_all_inplace(tmpl, "{{REGISTRATIONS}}", registrations.str());

        std::ofstream out(output_path.string() + "/register_js_binding.cpp");
        out << tmpl;
        out.close();

        std::cout << "[CodeGenerator] Generated register_js_binding.cpp" << std::endl;
    }

    void CodeGenerator::GenerateJSTypesJS(const std::vector<ClassParseResult>& class_results)
    {
        std::string tmpl = CodeGenUtils::read_template("js_types.js.tmpl");
        if (tmpl.empty())
            return;

        std::stringstream js_classes;

        for (const auto& cls : class_results)
        {
            bool has = false;
            for (const auto& f : cls.field_results)
                if (f.has_js_binding)
                {
                    has = true;
                    break;
                }
            if (!has)
                for (const auto& m : cls.method_results)
                    if (m.has_js_binding)
                    {
                        has = true;
                        break;
                    }
            if (!has)
                continue;

            js_classes << "class " << cls.class_name << " {\n";
            js_classes << "    #ptr;\n";
            js_classes << "    constructor(ptr) { this.#ptr = ptr; }\n\n";

            for (const auto& f : cls.field_results)
            {
                if (!f.has_js_binding)
                    continue;
                js_classes << "    get " << f.field_name << "()  { return __get_" << cls.class_name << "_"
                           << f.field_name << "(this.#ptr); }\n";
                js_classes << "    set " << f.field_name << "(v) { __set_" << cls.class_name << "_" << f.field_name
                           << "(this.#ptr, v); }\n\n";
            }

            for (const auto& m : cls.method_results)
            {
                if (!m.has_js_binding)
                    continue;
                js_classes << "    " << m.method_name << "() { return __call_" << cls.class_name << "_" << m.method_name
                           << "(this.#ptr); }\n";
            }

            js_classes << "}\n\n";
        }

        CodeGenUtils::replace_all_inplace(tmpl, "{{JS_CLASSES}}", js_classes.str());

        std::ofstream out(output_path.string() + "/js_types.js");
        out << tmpl;
        out.close();

        std::cout << "[CodeGenerator] Generated js_types.js" << std::endl;
    }

    void CodeGenerator::End()
    {
        if (!is_recording)
        {
            std::cerr << "Parser already ends recording." << std::endl;
            return;
        }

        is_recording = false;
    }

    void CodeGenerator::GenerateEnumReflHeaderFile(const EnumParseResult& enum_result)
    {
        std::string tmpl = CodeGenUtils::read_template("enum_gen.h.tmpl");
        if (tmpl.empty())
        {
            // fallback inline template
            tmpl = R"(#pragma once

#include "core/reflect/macros.h"

#include <cstdint>
#include <string>

namespace Meow
{
    enum class EnumName : UnderlyingType;

    EnumName to_enum(const std::string& str);

    const std::string to_string(EnumName enum_val);
} // namespace Meow
)";
        }

        CodeGenUtils::replace_all_inplace(tmpl, "EnumName", enum_result.enum_name);
        CodeGenUtils::replace_all_inplace(tmpl, "UnderlyingType", enum_result.underlying_type_name);

        std::string   gen_header_file_name = CodeGenUtils::camel_case_to_under_score(enum_result.enum_name);
        std::ofstream output_header_file(output_path.string() + "/" + gen_header_file_name + ".gen.h");
        if (output_header_file.is_open())
        {
            output_header_file << tmpl;
            output_header_file.close();
            std::cout << "[CodeGenerator] Generated: " << output_path.string() + "/" + gen_header_file_name + ".gen.h"
                      << std::endl;
        }
        else
        {
            std::cerr << "[CodeGenerator] Fail to write: "
                      << output_path.string() + "/" + gen_header_file_name + ".gen.h" << std::endl;
        }
    }
} // namespace Meow
