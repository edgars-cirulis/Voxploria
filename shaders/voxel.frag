#version 450

layout(location=0) in vec3 vWorld;
layout(location=1) in vec3 vN;
layout(location=2) in vec4 vTile;   
layout(location=3) in vec2 vAO;

layout(location=0) out vec4 outColor;

layout(binding=0) uniform sampler2D uAtlas;

layout(push_constant) uniform Push {
    mat4 uMVP;
    vec4 uLight;   
    vec4 uCamPos;  
} pc;

float saturate(float x){ return clamp(x, 0.0, 1.0); }

vec2 facePlaneCoords(vec3 wp, vec3 n){
    vec3 an = abs(n);
    if (an.x > an.y && an.x > an.z) {
        float u = (n.x > 0.0) ? wp.z : -wp.z;
        float v = -wp.y;                    
        return vec2(u, v);
    } else if (an.y > an.z) {
        float u = wp.x;
        float v = (n.y > 0.0) ? -wp.z :  wp.z; 
        return vec2(u, v);
    } else {
        float u = (n.z > 0.0) ? wp.x : -wp.x;
        float v = -wp.y;                    
        return vec2(u, v);
    }
}

vec2 tileInRect(vec2 uvRepeat01, vec4 rect){
    vec2 size = rect.zw - rect.xy;
    vec2 texSz = vec2(textureSize(uAtlas, 0));
    vec2 texel = 1.0 / texSz;
    vec2 margin = max(2.0 * texel, size * 0.01);
    vec2 lo = rect.xy + margin;
    vec2 hi = rect.zw - margin;
    return mix(lo, hi, fract(uvRepeat01));
}

vec3 skyColor(){
    float t = pc.uLight.w;
    vec3 base = vec3(0.60, 0.78, 0.96);
    base += vec3(0.015*sin(t*0.07), 0.01*sin(t*0.05), 0.0);
    return base;
}

void main(){
    vec3 N = normalize(vN);
    vec3 L = normalize(-pc.uLight.xyz);   
    vec3 V = normalize(pc.uCamPos.xyz - vWorld);

    vec2 uvPlane = facePlaneCoords(vWorld, N);
    vec2 uv      = tileInRect(uvPlane, vTile);

    vec3 albedo = textureLod(uAtlas, uv, 0.0).rgb;

    float ndotl = max(dot(N, L), 0.0);

    vec3 skyAmb    = vec3(0.34, 0.42, 0.50);
    vec3 groundAmb = vec3(0.09, 0.08, 0.07);  
    float up = N.y * 0.5 + 0.5;
    vec3 ambient = mix(groundAmb, skyAmb, saturate(up));

    float ambK = 0.32;    
    float sunK = 0.95;    
    
    float ao = clamp(vAO.x, 0.0, 1.0) * clamp(vAO.y, 0.0, 1.0);

    vec3 lit = albedo * (ambient * ambK) * ao
             + albedo * (ndotl * sunK) * mix(0.85, 1.0, ao);

    float dist = length(vWorld - pc.uCamPos.xyz);
    float fog  = smoothstep(200.0, 360.0, dist);
    vec3  sky  = skyColor();
    vec3  color = mix(lit, sky, fog);

    outColor = vec4(color, 1.0);
}
