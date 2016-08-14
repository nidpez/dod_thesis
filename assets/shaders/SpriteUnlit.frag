#version 330 core

uniform sampler2D tex;

in vec2 interpTexCoords;

out vec4 finalColor;

void main() {
  //TODO: do lighting calculations
  //...
  
  finalColor = texture( tex, interpTexCoords );
}
