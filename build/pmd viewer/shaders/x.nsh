@sampler base "diffuse"
@sampler env_add "env add"
@sampler env_mult "env mult"

@uniform light_dir "light dir"=0.4,0.82,0.4
@uniform light_color "light color"=0.6,0.6,0.6
@predefined cam_pos "nya camera position":local

@uniform amb_k "amb k"
@uniform diff_k "diff k"
@uniform spec_k "spec k"

@all
varying vec2 tc;
varying vec2 env_tc;
varying vec3 normal;
varying vec3 pos;

@vertex

void main()
{
    tc=gl_MultiTexCoord0.xy;
    
    pos=gl_Vertex.xyz;
    normal=gl_Normal;

    vec3 r=normalize((gl_ModelViewProjectionMatrix * vec4(normal,0.0)).xyz);
    r.z-=1.0;
    env_tc=0.5*r.xy/length(r) + 0.5;

    gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
}

@fragment
uniform sampler2D base;
uniform sampler2D env_add;
uniform sampler2D env_mult;

uniform vec4 light_dir;
uniform vec4 light_color;
uniform vec4 cam_pos;

uniform vec4 amb_k;
uniform vec4 diff_k;
uniform vec4 spec_k;

void main()
{   
    vec4 c = texture2D(base,tc);
    c.a *= diff_k.a;
    if(c.a < 0.01)
        discard;

    c.rgb *= texture2D(env_mult, env_tc).rgb;
    c.rgb += texture2D(env_add, env_tc).rgb;

    vec3 eye = normalize(cam_pos.xyz - pos);
    float ndh = dot(normal, normalize(light_dir.xyz + eye));
    vec3 spec = spec_k.rgb * max(pow(ndh, spec_k.a), 0.0);

    c.rgb *= clamp(amb_k.rgb + (diff_k.rgb + spec) * light_color.rgb, vec3(0.0), vec3(1.0));

    gl_FragColor = c;
}
