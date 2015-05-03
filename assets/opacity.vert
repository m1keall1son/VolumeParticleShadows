#version 330

in vec2 ciTexCoord0;
in vec4 ciPosition;

uniform sampler2D uParticleTexture;

uniform mat4 ciViewMatrix;
uniform mat4 ciModelMatrix;
uniform mat4 ciProjectionMatrix;

uniform float uParticleDiameter;
uniform float uScreenWidth;

void main () {
    
    vec4 data = texture(uParticleTexture, ciTexCoord0.xy);
    vec3 position = data.rgb;
    
    vec3 viewSpacePosition = vec3(ciViewMatrix * vec4(position, 1.0));
    
    vec4 corner = vec4(uParticleDiameter * 0.5, uParticleDiameter * 0.5, viewSpacePosition.z, 1.0);
    
    float projectedCornerX = dot(vec4(ciProjectionMatrix[0][0],
                                      ciProjectionMatrix[1][0],
                                      ciProjectionMatrix[2][0],
                                      ciProjectionMatrix[3][0]
                                      )
                                 , corner);
    
    float projectedCornerW = dot(vec4(ciProjectionMatrix[0][3],
                                      ciProjectionMatrix[1][3],
                                      ciProjectionMatrix[2][3],
                                      ciProjectionMatrix[3][3]
                                      )
                                 , corner);
    
    gl_PointSize = uScreenWidth * 0.5 * projectedCornerX * 2.0 / projectedCornerW * pow( data.a/4., .1);
    
    gl_Position = ciProjectionMatrix * vec4(viewSpacePosition, 1.0);
}