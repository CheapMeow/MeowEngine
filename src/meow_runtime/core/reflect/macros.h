#pragma once

#define reflectable_class(...)  clang::annotate("reflectable_class;" #__VA_ARGS__)
#define reflectable_struct(...) clang::annotate("reflectable_struct;" #__VA_ARGS__)
#define reflectable_field(...)  clang::annotate("reflectable_field;" #__VA_ARGS__)
#define reflectable_method(...) clang::annotate("reflectable_method;" #__VA_ARGS__)
#define reflectable_enum(...)   clang::annotate("reflectable_enum;" #__VA_ARGS__)
