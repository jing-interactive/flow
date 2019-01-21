#define PI 3.14159265359

// Plot a line on X using a value between 0.0-1.0
float plotOnX(vec2 st, float pct, float k){
  return  smoothstep( pct-k, pct, st.x) -
          smoothstep( pct, pct+k, st.x);
}

// Plot a line on Y using a value between 0.0-1.0
float plotOnY(vec2 st, float pct, float k){
  return  smoothstep( pct-k, pct, st.y) -
          smoothstep( pct, pct+k, st.y);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 st = fragCoord/iResolution.xy;
    st -= 0.5;
    st.x *= iResolution.x/iResolution.y;

    float cut = 0.01; //iMouse.y * 100 / iResolution.x;
    float x = 0;
    vec3 colorX = plotOnX(st, x, cut)*vec3(1.0);
    // float y = step(0.5, st.x); // y = f(x)
    // float y = smoothstep(0.3,0.5,st.x) - smoothstep(0.5,0.7,st.x);
    float y = sin(st.x * 20.0 + iTime) / 5;
    vec3 colorY = plotOnY(st, y, cut)*vec3(1.0);

    fragColor = vec4(colorX + colorY,1.0);
}