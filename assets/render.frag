#version 330

in vec3 vPosition;
in float vOpacity;

in vec2 vTexCoord0;

uniform float uParticleAlpha;
uniform vec3 uParticleColor;
//uniform bool uFlipped; //non-flipped is front-to-back, flipped is back-to-front

out vec4 FragColor;

void main () {
    
   // FragColor = vec4( vTexCoord0, 1., 1.);
    
    float distanceFromCenter = distance( gl_PointCoord.xy, vec2(0.5, 0.5) );
    
    if (distanceFromCenter > 0.5) discard;
    
    float alpha = clamp(1.0 - distanceFromCenter * 2.0, 0.0, 1.0) * uParticleAlpha;
    
    vec3 color = (1.0 - vOpacity * .75 ) * uParticleColor;
    
    //premultiply alpha
    FragColor = vec4(color * alpha, alpha);
    
}