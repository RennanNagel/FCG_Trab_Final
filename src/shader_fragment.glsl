#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Uniform para controlar a transparência
uniform float transparency = 1.0;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Material uniforms
uniform vec3 kd;
uniform vec3 ka;
uniform vec3 ks;
uniform float q;

// Identificador que define qual objeto está sendo desenhado no momento
#define SPHERE     0
#define BUNNY      1
#define PLANE      2
#define GHOST      3
#define MAZE       4
#define ENEMY_RED  5
#define ENEMY_BLUE 6

uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
uniform sampler2D TextureImage5;

uniform vec4 fog_color;    
uniform float fog_density; 


// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define espectro da fonte de luz 
    vec3 I = vec3(1.0, 1.0, 1.0);

    vec3 Ia = vec3(1.0, 1.0, 1.0);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;
    float ao = 1.0;
    vec3 Kd0 = vec3(0.0, 0.0, 0.0);

    if ( object_id == SPHERE )
    {
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;
        vec4 p_prime = bbox_center + (position_model - bbox_center)/length(position_model - bbox_center);
        vec4 p = p_prime - bbox_center;
        float theta = atan(p.x, p.z);
        float phi = asin(p.y);

        U = (theta + M_PI)/(2*M_PI);
        V = (phi + M_PI_2)/M_PI;
        ao = texture(TextureImage0, vec2(U, V)).r;
        Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    }

    else if ( object_id == BUNNY )
    {
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x - minx) / (maxx - minx);
        V = (position_model.y - miny) / (maxy - miny);
        Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    }

    else if ( object_id == PLANE )
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x*50;
        V = texcoords.y*50;
        n = vec4(normalize(texture(TextureImage1, vec2(U, V)).rgb), 0.0f);
        Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
        ao = texture(TextureImage1, vec2(U, V)).r;
    }

    else if ( object_id == MAZE ) 
    {
        U = texcoords.x;
        V = texcoords.y;
        Kd0 = normalize(texture(TextureImage2, vec2(U, V)).rgb);
    }

    // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
    // vec3 Kd0 = texture(TextureImage0, vec2(U,V)).rgb;

    // if (object_id == SPHERE) 
    // {
    //     Kd0 += min(50, max(-100*dot(n,l), 0))*texture(TextureImage1, vec2(U, V)).rgb;
    // }
    
    // vec4 n = vec4(normalize(texture(TextureImage1, vec2(U, V)).rgb), 0.0f);

    if ( object_id == GHOST ) 
    {
        U = texcoords.x;
        V = texcoords.y;
        Kd0 = texture(TextureImage3, vec2(U, V)).rgb;
    }

    if ( object_id == ENEMY_RED ) 
    {
        U = texcoords.x;
        V = texcoords.y;
        Kd0 = texture(TextureImage4, vec2(U, V)).rgb;
    }

    if ( object_id == ENEMY_BLUE ) 
    {
        U = texcoords.x;
        V = texcoords.y;
        Kd0 = texture(TextureImage5, vec2(U, V)).rgb;
    }

        // Iluminação Phong
    float lambert = max(0,dot(n,l));
    vec4 r = -l + 2*n*dot(n,l);
    
    vec3 base_color = Kd0 * I * (lambert + 0.01) + ks*I*pow(max(0,dot(r, v)), q);
    base_color = pow(base_color, vec3(1.0,1.0,1.0)/2.2); // Gamma correction
    
    // === FOG CALCULO ===
    float distance = length(p.xyz - camera_position.xyz); 
    float fogFactor = exp(-pow((distance * fog_density), 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    
    vec3 final_color = mix(fog_color.rgb, base_color, fogFactor); 
    
    color = vec4(final_color, transparency);


} 

