//
// Created by marc on 16/7/26.
//

// 1. Compilamos la implementación de TinyObjLoader primero
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// 2. Compilamos la implementación de TinyGLTF permitiendo que use STB_IMAGE
//    ¡OJO! Quitamos las macros de NO_STB.
//    En su lugar, definimos las macros de STB directamente aquí para que
//    las use a través de su propia inclusión interna de forma aislada.
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>