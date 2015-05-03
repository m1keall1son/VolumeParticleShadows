#version 330

//in vec4 ciPosition;
//in vec2 ciTexCoord0;
//uniform mat4 ciModelViewProjection;
//
//out vec2 vTexCoord0;
//
//uniform sampler2D uPositions;
//
//void main () {
//    vTexCoord0 = ciTexCoord0;
//    
//    vec3 position = texture(uPositions, ciTexCoord0).xyz;
//    
//    gl_Position = ciModelViewProjection * vec4( position, 1. );
//}

//#version 330

in vec4 ciPosition;
in vec2 ciTexCoord0;

out vec3 vPosition;
out float vOpacity;
out vec2 vTexCoord0;

uniform sampler2D uParticleTexture;

uniform sampler2D uOpacityTexture;

uniform mat4 ciViewMatrix;
uniform mat4 ciModelMatrix;
uniform mat4 ciProjectionMatrix;
uniform mat4 ciModelViewProjection;

uniform mat4 uLightViewProjection;

uniform float uParticleDiameter;
uniform float uScreenWidth;

//const mat4 biasMat  = mat4(	0.5, 0.0, 0.0, 0.0,
//                           0.0, 0.5, 0.0, 0.0,
//                           0.0, 0.0, 0.5, 0.0,
//                           0.5, 0.5, 0.5, 1.0 );

void main () {
    
    vTexCoord0 = ciTexCoord0;
    
    vec4 data = texture(uParticleTexture, ciTexCoord0.xy);
    
    vec3 position = data.rgb;
    
    vPosition = position;
    
    vec2 lightTextureCoordinates = vec2(uLightViewProjection *  vec4(position, 1.0) ) * 0.5 + 0.5;
    
    //biasMat * uLightViewProjection * ciModelMatrix * vec4(position, 1.0)
    
    vOpacity = texture(uOpacityTexture, lightTextureCoordinates ).a;
    
    vec3 viewSpacePosition = vec3( ciViewMatrix  *  vec4( position, 1.0 ) );
    
    vec4 corner = vec4(uParticleDiameter * 0.5, uParticleDiameter * 0.5, viewSpacePosition.z, 1.0);
    
    float projectedCornerX = dot( vec4(ciProjectionMatrix[0][0],
                                       ciProjectionMatrix[1][0],
                                       ciProjectionMatrix[2][0],
                                       ciProjectionMatrix[3][0]
                                       )
                                 , corner );
    
    float projectedCornerW = dot( vec4(ciProjectionMatrix[0][3],
                                       ciProjectionMatrix[1][3],
                                       ciProjectionMatrix[2][3],
                                       ciProjectionMatrix[3][3]
                                       )
                                 , corner);
    
    gl_PointSize = uScreenWidth * 0.5 * projectedCornerX * 2.0 / projectedCornerW * smoothstep(0.,1., data.a/4.);
    
    gl_Position = ciProjectionMatrix * vec4(viewSpacePosition, 1.0);
    
    if (position.y < -.25 ) gl_Position = vec4(9999999.0, 9999999.0, 9999999.0, 1.0); //less than floor origin
    
}