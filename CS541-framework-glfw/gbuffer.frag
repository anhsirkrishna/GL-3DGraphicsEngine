/////////////////////////////////////////////////////////////////////////
// Pixel shader for Final output
////////////////////////////////////////////////////////////////////////
#version 330

// These definitions agree with the ObjectIds enum in scene.h
const int     nullId	= 0;
const int     skyId	= 1;
const int     seaId	= 2;
const int     groundId	= 3;
const int     roomId	= 4;
const int     boxId	= 5;
const int     frameId	= 6;
const int     lPicId	= 7;
const int     rPicId	= 8;
const int     teapotId	= 9;
const int     spheresId	= 10;
const int     floorId	= 11;

const float PI = 3.14159f;

uniform int objectId;
uniform int lightingMode;
uniform int textureMode;
uniform vec3 diffuse, specular;
uniform float shininess;
uniform vec3 light, ambient;
uniform int width, height;
uniform bool reflective;

uniform sampler2D shadowMap;
uniform sampler2D SkydomeTex;
uniform sampler2D IrrMapTex;
uniform sampler2D ObjectTexture;
uniform sampler2D ObjectNMap;
uniform int hasTexture, hasNMap;

in vec3 normalVec, lightVec, eyeVec;
in vec2 texCoord;
in vec3 tanVec;
in vec4 worldPos;

void main()
{   
    vec3 N = normalize(normalVec);
    vec3 L = normalize(lightVec);
    vec3 V = normalize(eyeVec);
    vec3 H = normalize(L+V);
	vec3 Kd = diffuse;
    vec3 Ks = specular;

    if (textureMode == 1) {

        if (objectId == skyId){
            vec2 uv = vec2(-atan(V.y, V.x)/(2*PI), acos(V.z)/PI);
            Kd = texture2D(SkydomeTex, uv).xyz;
            gl_FragData[1].w = 1;
            gl_FragData[2].xyz = Kd;
            return;
        }
        else if (objectId == seaId){
            vec3 R = -(2* dot(V, N) * N - V);
            vec2 uv = vec2(-atan(R.y, R.x)/(2*PI), acos(R.z)/PI);
            Kd = texture2D(SkydomeTex, uv).xyz;
        }

        if (hasTexture != -1){
            //Get color from texture
            vec2 uv = texCoord;

            if (objectId == groundId)
                uv *= 10;
            if (objectId == floorId)
                uv *= 5;
            if (objectId == roomId){
                uv = uv.yx;
                uv *= 50;
            }
            if (objectId == rPicId){
                if (texCoord.x < 0.1){
                    Kd = vec3(0.7, 0.7, 0.7);
                }
                else if (texCoord.y < 0.1){
                    Kd = vec3(0.7, 0.7, 0.7);
                }
                else if (texCoord.x > 0.9){
                    Kd = vec3(0.7, 0.7, 0.7);
                }
                else if (texCoord.y > 0.9){
                    Kd = vec3(0.7, 0.7, 0.7);
                }  
                else{
                    uv.x -= 0.05;
                    uv.y -= 0.1;
                    uv /= 0.8;
                    Kd = texture2D(ObjectTexture, uv).xyz;
                }
            }
            else if (objectId == lPicId){
                uv *= 100;
                if ((mod(uv.x, 20) > 5) && mod(uv.y, 20) > 5)
                    Kd = vec3(0.6, 0.0, 0.9);
                else if ( uv.x < (uv.y + 10) && uv.x > (uv.y - 10))
                    Kd = vec3(0,0,0);
                else
                    Kd = vec3(0.3, 0.0, 0.0);
            }
            else
                Kd = texture2D(ObjectTexture, uv).xyz;
        }
        if (hasNMap != -1){
            vec2 uv = texCoord;
            if (objectId == groundId)
                uv *= 10;
            if (objectId == floorId)
                uv *= 5;
            if (objectId == roomId){
                uv = uv.yx;
                uv *= 50;
            }
            if (objectId == seaId){
                uv = uv.yx;
                uv *= 500;
            }
            vec3 delta = texture2D(ObjectNMap, uv).xyz;
            if (objectId == floorId)
                delta = delta/(100.0/255.0); //Some extra calculation required for the special normal map used for the floor
            delta = delta*2.0 - vec3(1, 1, 1);
            vec3 T = normalize(tanVec);
            vec3 B = normalize(cross(T, N));
            N = delta.x*T + delta.y*B + delta.z*N;
        }
    }

    // A checkerboard pattern to break up larte flat expanses.  Remove when using textures.
    if (textureMode == 0){
        if (objectId==groundId || objectId==floorId || objectId==seaId) {
            ivec2 uv = ivec2(floor(100.0*texCoord));
            if ((uv[0]+uv[1])%2==0)
                Kd *= 0.9; }
    }

	gl_FragData[0] = worldPos;
	gl_FragData[1].xyz = N;
    gl_FragData[1].w = 0;
	gl_FragData[2].xyz = Kd;
	gl_FragData[3].xyz = Ks;
	gl_FragData[3].w = shininess;
	if (reflective)
		gl_FragData[2].w = 1;
	else
		gl_FragData[2].w = 0;
}
