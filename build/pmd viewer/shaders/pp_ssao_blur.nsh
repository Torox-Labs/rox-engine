@all

varying vec2 tc;

@vertex

void main()
{
    tc=gl_MultiTexCoord0.xy;
    gl_Position=gl_Vertex;
}

@sampler base_map "diffuse"

@predefined vp "nya viewport"

@fragment

uniform sampler2D base_map;
uniform vec4 vp;

void main()
{
    int blur_size = 4;

    vec2 texel_size = 1.0 / vp.zw;
    float result = 0.0;
    vec2 hlim = vec2(float(-blur_size) * 0.5 + 0.5);
    for (int x = 0; x < blur_size; ++x)
    {
        for (int y = 0; y < blur_size; ++y)
        {
            vec2 offset = (hlim + vec2(float(x), float(y))) * texel_size;
            result += texture2D(base_map, tc + offset).r;
        }
    }

    gl_FragColor=vec4(result / float(blur_size * blur_size));
}
