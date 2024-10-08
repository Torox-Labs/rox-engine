@sampler morph "morph"
@uniform morphs "morphs"
@uniform morphs_count "morphs count"=0,1

@vertex
uniform sampler2D morph;
uniform vec4 morphs_count;
uniform vec4 morphs[16];

vec3 apply_morphs(vec3 p,float vidx)
{
    for(int i=0;i<int(morphs_count.x);++i)
    {
        vec2 mtc;
        mtc.x=vidx+morphs[i].y;
        mtc.y=floor(mtc.x/morphs_count.y);
        mtc.x-=mtc.y*morphs_count.y;

        vec3 mv = texture2D(morph,(mtc+vec2(0.5))/morphs_count.yz).xyz;
            p += mv * morphs[i].x;
    }

    return p;
}

vec2 apply_morphs(vec2 uv,float vidx)
{
    for(int i=int(morphs_count.x);i<int(morphs_count.w);++i)
    {
        vec2 mtc;
        mtc.x=vidx+morphs[i].y;
        mtc.y=floor(mtc.x/morphs_count.y);
        mtc.x-=mtc.y*morphs_count.y;

        vec2 mv = texture2D(morph,(mtc+vec2(0.5))/morphs_count.yz).xy;
        uv += mv * morphs[i].x;
    }

    return uv;
}
