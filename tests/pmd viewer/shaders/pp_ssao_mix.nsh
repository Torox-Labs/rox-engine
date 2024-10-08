@all

varying vec2 tc;

@vertex

void main()
{
    tc=gl_MultiTexCoord0.xy;
    gl_Position=gl_Vertex;
}

@sampler ssao_map "ssao"
@sampler base_map "base"
@predefined vp "nya viewport"

@fragment

uniform sampler2D ssao_map;
uniform sampler2D base_map;

uniform vec4 vp;

vec3 rgb2ycbcr(vec3 color)
{
    return vec3(dot(color,vec3(0.298912,0.586611,0.114478)),
                dot(color,vec3(-0.168736,-0.331264,0.5)),
                dot(color,vec3(0.5,-0.418688,-0.081312)));
}

vec3 ycbcr2rgb(vec3 ycbcr)
{
    return vec3(dot(ycbcr,vec3(1.0,-0.000982,1.401845)),
                dot(ycbcr,vec3(1.0,-0.345117,-0.714291)),
                dot(ycbcr,vec3(1.0,1.771019,-0.000154)));
}

void main()
{
    float ssao = texture2D(ssao_map,tc).r;
    vec2 texel_size = 1.0 / vp.zw;
    ssao += texture2D(ssao_map,tc+texel_size*vec2(0.0,-1.0)).r;
    ssao += texture2D(ssao_map,tc+texel_size*vec2(0.0,-1.0)).r;
    ssao += texture2D(ssao_map,tc+texel_size*vec2(0.0,1.0)).r;
    ssao += texture2D(ssao_map,tc+texel_size*vec2(-1.0,0.0)).r;
    ssao += texture2D(ssao_map,tc+texel_size*vec2(1.0,0.0)).r;
    ssao += texture2D(ssao_map,tc+texel_size*vec2(1.0,-1.0)).r;
    ssao += texture2D(ssao_map,tc+texel_size*vec2(-1.0,1.0)).r;
    ssao += texture2D(ssao_map,tc+texel_size*vec2(1.0,1.0)).r;
    ssao *= 0.1;

    vec4 color=texture2D(base_map,tc);

    vec3 ycbcr=rgb2ycbcr(color.rgb);
    float a = clamp(1.0 - 0.5 * (1.0-ssao), 0.1, 1.0);
    ycbcr.r *= a;
    vec3 c = mix(color.rgb*a, color.rgb, pow(color.rgb, vec3(1.0/a)));
    color.rgb = mix(ycbcr2rgb(ycbcr), c, 0.8);

    //float value = 0.3;
	//color.rgb = mix(1.0, ssao, value) * color.rgb;
    gl_FragColor = color;
}
