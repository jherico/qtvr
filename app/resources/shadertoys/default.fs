void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
  vec2 uv = fragCoord.xy / iResolution.xy;
  vec4 color = vec4(iDir, 1.0);
//  vec4 color = texture(iChannel0, iDir);
  fragColor = color; 
}
