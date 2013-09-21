
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glfw.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

const char* vertex_src = "\
#version 150\
\n\
\
in vec2 position;\
in vec2 texturepos;\
out vec2 Texturepos;\
void main()\
{\
    gl_Position = vec4( position, 0.0, 1.0 );\
    Texturepos = texturepos;\
}\
";

const char* fragment_src = "\
#version 150\
\n\
in vec2 Texturepos;\
out vec4 outColor;\
uniform sampler2D tex;\
void main()\
{\
	vec4 t = texture( tex, Texturepos );\
	outColor = t;\
}\
";

#define XRES (128)
#define YRES (128)
#define PIXEL_SIZE (4)

float vertices[] = {
	-1.0f,  1.0f, 0, 0, // Vertex 1 (X, Y)
	1.0f, -1.0f, 1, 1,// Vertex 2 (X, Y)
	-1.0f, -1.0f, 0, 1,  // Vertex 3 (X, Y)
	-1.0f,  1.0f, 0, 0, // Vertex 1 (X, Y)
	1.0f, 1.0f, 1, 0,// Vertex 2 (X, Y)
	1.0f, -1.0f, 1, 1,  // Vertex 3 (X, Y)
};

bool initiated = false;

GLuint vbo;
GLuint shaderProgram;
GLuint tex;
int display_init()
{
	glfwInit();
	glfwOpenWindowHint( GLFW_OPENGL_VERSION_MAJOR, 3 );
	glfwOpenWindowHint( GLFW_OPENGL_VERSION_MINOR, 2 );
	glfwOpenWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

	glfwOpenWindowHint( GLFW_WINDOW_NO_RESIZE, GL_TRUE );
	glfwOpenWindow( XRES*PIXEL_SIZE, YRES*PIXEL_SIZE, 0, 0, 0, 0, 0, 0, GLFW_WINDOW );
	glfwSetWindowTitle( "OpenGL" );

	glewExperimental = GL_TRUE;
	glewInit();

	glGenBuffers( 1, &vbo ); // Generate 1 buffer

	GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( vertexShader, 1, &vertex_src, NULL );
	glCompileShader( vertexShader );
	GLint status;
	glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &status );
	if(status != GL_TRUE)
		return 1;
	char buffer[512];
	glGetShaderInfoLog( vertexShader, 512, NULL, buffer ); 
	printf(buffer);

	GLuint fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( fragmentShader, 1, &fragment_src, NULL );
	glCompileShader( fragmentShader );
	glGetShaderInfoLog( fragmentShader, 512, NULL, buffer ); 
	printf(buffer);
	
	shaderProgram = glCreateProgram();
	glAttachShader( shaderProgram, vertexShader );
	glAttachShader( shaderProgram, fragmentShader );

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindFragDataLocation( shaderProgram, 0, "outColor" );
	glLinkProgram( shaderProgram );
	glfwSwapInterval(1);

	float v[1000*3];
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	glBufferData( GL_ARRAY_BUFFER, sizeof( v ), v, GL_DYNAMIC_DRAW );
	initiated = true;
	return 0;
}

void display_close()
{
	initiated = false;
	glfwTerminate();
}

unsigned char buffer[XRES*YRES*3];
unsigned char cols[4*3] = {0xff, 0xaa, 0x77, 0x22};
int display(unsigned char *data)
{ 
	if(!initiated)
		return 0;
	
	if(!glfwGetWindowParam( GLFW_OPENED ))
		return 1;
	if ( glfwGetKey( GLFW_KEY_ESC ) == GLFW_PRESS )
		return 1;

	glClearColor(0.204f, 0.243f, 0.275f, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int i;
	for(i=0; i<XRES*YRES; i++)
	{
		int index = i/4;
		int offset = i%4;
		int col = data[index]&(0x3<<offset);
		buffer[i*3] = cols[col];
		buffer[i*3+1] = cols[col];
		buffer[i*3+2] = cols[col];
	}
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_DYNAMIC_DRAW );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, tex );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, XRES, YRES, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);

	glUniform1i( glGetUniformLocation( shaderProgram, "tex" ), 0 );

	glUseProgram( shaderProgram );

#define BUFFER_OFFSET(i) ((void*)(i))
	GLint posAttrib = glGetAttribLocation( shaderProgram, "position" );
	glVertexAttribPointer( posAttrib, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0 );
	glEnableVertexAttribArray( posAttrib );

	GLint texturePosAttrib = glGetAttribLocation( shaderProgram, "texturepos" );
	glVertexAttribPointer( texturePosAttrib, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), BUFFER_OFFSET(sizeof(float)*2) );
	glEnableVertexAttribArray( texturePosAttrib );

	glDrawArrays( GL_TRIANGLES, 0, 6 );
	glfwSwapBuffers();
	return 0;
}
