attribute vec4 Position;
attribute vec4 SourceColor;
 
varying vec4 DestinationColor;
 
void main(void) 
{
    //mat4 m = mat4(1.0) // initializing the diagonal of the matrix with 1.0
    mat4 m = mat4(1.0,0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, -40.0, 1.0);
    DestinationColor = SourceColor;
    //gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    //gl_Position = ftransform();
    gl_Position = m * Position;
}