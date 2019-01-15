uniform vec3      iResolution; // viewport resolution (in pixels)
uniform float     iTime; // shader playback time (in seconds)
uniform float     iTimeDelta; // render time (in seconds)
uniform int       iFrame; // render time (in seconds)
uniform float     iChannelTime[4]; // channel playback time (in seconds)
uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)
uniform vec4      iMouse; // mouse pixel coords. xy: current (if MLB down), zw: click
uniform sampler2D iChannel0; // input channel 0 (TODO: support samplerCube)
uniform sampler2D iChannel1; // input channel 1 (TODO: support samplerCube)
uniform sampler2D iChannel2; // input channel 2 (TODO: support samplerCube)
uniform sampler2D iChannel3; // input channel 3 (TODO: support samplerCube)
uniform vec4      iDate; // (year, month, day, time in seconds)
uniform float     iSampleRate;

void mainImage( out vec4 fragColor, in vec2 fragCoord );

void main(void)
{
    mainImage(gl_FragColor, gl_FragCoord.xy);
}

#define iGlobalTime iTime
