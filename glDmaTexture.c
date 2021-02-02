///-----------------------------------------------///
///	Example program for setting up a hardware     ///
/// path for camera/video over v4l2 to Open GL    ///
/// texture.						 		  	  ///
///-----------------------------------------------///
//
// sudo apt-get install libglfw3-dev libgles2-mesa-dev

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <linux/videodev2.h>
#include <errno.h>
#include <fcntl.h>
#include <drm_fourcc.h>

#include "glhelp.h"

static const GLuint WIDTH = 1280;
static const GLuint HEIGHT = 720;

static const GLchar* vertex_shader_source =
	"#version 300 es\n"
	"in vec3 position;\n"
	"in vec2 tx_coords;\n"
	"out vec2 v_texCoord;\n"
	"void main() {\n"
	"   gl_Position = vec4(position, 1.0);\n"
	"	v_texCoord = tx_coords;  \n"
	"}\n";
	
static const GLchar* fragment_shader_source =
	"#version 300 es\n"
	"#extension GL_OES_EGL_image_external : require\n"
	"precision mediump float;\n"
	"uniform samplerExternalOES texture;\n"
	"in vec2 v_texCoord;\n"
	"out vec4 out_color;\n"
	"void main() {	\n"
	"	out_color = texture2D( texture, v_texCoord );\n"
	"}\n";

/// negative x,z is bottom left and first vertex
static const GLfloat vertices[][4][3] =
{
    { {-1.0, -1.0, 0.0}, { 1.0, -1.0, 0.0}, {-1.0, 1.0, 0.0}, {1.0, 1.0, 0.0} }
};
static const GLfloat uv_coords[][4][2] =
{
    { {0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0} }
};

GLint common_get_shader_program(const char *vertex_shader_source, const char *fragment_shader_source) {
	enum Consts {INFOLOG_LEN = 512};
	GLchar infoLog[INFOLOG_LEN];
	GLint fragment_shader;
	GLint shader_program;
	GLint success;
	GLint vertex_shader;

	/* Vertex shader */
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex_shader, INFOLOG_LEN, NULL, infoLog);
		printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
	}

	/* Fragment shader */
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment_shader, INFOLOG_LEN, NULL, infoLog);
		printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
	}

	/* Link shaders */
	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);
	glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shader_program, INFOLOG_LEN, NULL, infoLog);
		printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return shader_program;
}

int main(void) {

	GLuint shader_program, vbo;
	GLint pos;
	GLint uvs;
	GLFWwindow* window;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	window = glfwCreateWindow(WIDTH, HEIGHT, __FILE__, NULL, NULL);
	glfwMakeContextCurrent(window);

	EGLDisplay egl_display = glfwGetEGLDisplay();
	if(egl_display == EGL_NO_DISPLAY) {
		printf("error: glfwGetEGLDisplay no EGLDisplay returned\n");
	}

	printf("GL_VERSION  : %s\n", glGetString(GL_VERSION) );
	printf("GL_RENDERER : %s\n", glGetString(GL_RENDERER) );

	shader_program = common_get_shader_program(vertex_shader_source, fragment_shader_source);
	pos = glGetAttribLocation(shader_program, "position");
	uvs = glGetAttribLocation(shader_program, "tx_coords");

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices)+sizeof(uv_coords), 0, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(uv_coords), uv_coords);
	glEnableVertexAttribArray(pos);
	glEnableVertexAttribArray(uvs);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glVertexAttribPointer(uvs, 2, GL_FLOAT, GL_FALSE, 0, sizeof(vertices)); /// last is offset to loc in buf memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	/// END Open GL setup ------------------------------------
	/// BEGIN V4L2 setup -------------------------------------
	
	int fd=-1;
	const char* camera_device = "/dev/video0";
	
	fd = open(camera_device, O_RDWR | O_NONBLOCK);
    if(fd == -1){
		printf("Cannot open device '%s'\n", camera_device);
		return -1;
    }
    printf("Camera device opened: %s with fd: %d\n", camera_device, fd);
	
	struct v4l2_capability cap;
	memset(&cap, 0, sizeof(cap));
	if( ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
	{
		if(EINVAL == errno){
			printf("VIDIOC_QUERYCAP: This isn't a V4L2 device\n");
		} else {
			perror("VIDIOC_QUERYCAP");
		}
		return -1;
	}
	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		printf("This is no video capture device\n");
		return -1;
	}
    if(!(cap.capabilities & V4L2_CAP_STREAMING))
    {
      printf("No streaming i/o support\n");
      return -1;
    }
	
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = WIDTH;
	fmt.fmt.pix.height      = HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGRX32; /// a yuv format might be faster but needs ISP unit?
	fmt.fmt.pix.field       = V4L2_FIELD_NONE;
	/// try setting this
	ioctl(fd, VIDIOC_S_FMT, &fmt);
	
	/// check what was actually set
	if(fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_BGRX32)
	{
		printf("Libv4l2 didn't accept the suggested pixel format. Can't proceed.\n");
		return -1;
	}
	if((fmt.fmt.pix.width != WIDTH) || (fmt.fmt.pix.height != HEIGHT))
	{
		printf("Warning: driver is sending image at %dx%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
	}
	
	printf("Device accepted fourcc:  %c%c%c%c\n",  /// RX24==BGRX32
	fmt.fmt.pix.pixelformat,
	fmt.fmt.pix.pixelformat >> 8,
	fmt.fmt.pix.pixelformat >> 16,
	fmt.fmt.pix.pixelformat >> 24);
	printf("Device accepted resolution:  %dx%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);

	/// VIDIOC_REQBUFS
	int buffer_count = 0;
	struct v4l2_requestbuffers reqbuf;
	memset(&reqbuf, 0, sizeof(reqbuf));
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = 1;
	int res;
	res = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
	if(res == -1 && errno == EINVAL)
	{
		reqbuf.count = 1;
		res = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
	}
	if(res == -1)
	{
		if(errno == EINVAL){
			printf("Video capturing or DMABUF streaming is not supported\n");
		} else {
			perror("VIDIOC_REQBUFS");
		}
		return -1;
	}
	buffer_count = reqbuf.count;
	printf("V4L2 Buffer Count:  %d\n", buffer_count);
	
	/// VIDIOC_EXPBUF
	/// you must do this for each of your V4L2 buffers (above).
	/// if you do double buffering for example
	int expBuf_fd = -1;
	struct v4l2_exportbuffer expbuf;
	memset(&expbuf, 0, sizeof(expbuf));
	expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	expbuf.index = 0; /// '0' for one buffer
	expbuf.flags = O_RDONLY;
	if(ioctl(fd, VIDIOC_EXPBUF, &expbuf) == -1)
	{
		perror("VIDIOC_EXPBUF"); // I solved with custom kernel fix - https://www.raspberrypi.org/forums/viewtopic.php?f=43&t=291940
		return -1;
	}
	expBuf_fd = expbuf.fd;
	printf("Dmabuf fd:  %d\n", expBuf_fd);
	
	/// Kick off the streaming
	if(ioctl(fd, VIDIOC_STREAMON, &(enum v4l2_buf_type){V4L2_BUF_TYPE_VIDEO_CAPTURE}))
	{
		perror("VIDIOC_STREAMON");
		return -1;
	}
	printf("Camera streaming turned ON\n");

	/// END getdmabuf() ------------------------------------------------
	/// BEGIN create DMA texture ---------------------------------------

	EGLImageKHR dma_image;
	dma_image = eglCreateImageKHR(	  
					egl_display,
					EGL_NO_CONTEXT,
					EGL_LINUX_DMA_BUF_EXT,
					NULL,
					(EGLint[])
					{
						EGL_WIDTH, fmt.fmt.pix.width,
						EGL_HEIGHT, fmt.fmt.pix.height,
						EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_XRGB8888,  /// takes 16 or 32 bits per pixel
						EGL_DMA_BUF_PLANE0_FD_EXT, expBuf_fd,
						EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
						EGL_DMA_BUF_PLANE0_PITCH_EXT, fmt.fmt.pix.bytesperline,
						EGL_NONE
					});

	if(dma_image == EGL_NO_IMAGE_KHR)
	{
		printf("error: eglCreateImageKHR failed\n");
		return -1;
	}


	GLuint dma_texture;
	glGenTextures(1, dma_texture);
	glEnable(GL_TEXTURE_EXTERNAL_OES);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, dma_texture);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, dma_image);
	glGetUniformLocation(shader_program, "texture");

	
	struct v4l2_buffer buf;
	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0;

	if(ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1)
	{
		perror("VIDIOC_QUERYBUF");
		return -1;
	}

	
	/// Main Program Loop
	///
	while(!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
				
		ioctl(fd, VIDIOC_DQBUF, &buf);
			glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, dma_image);
		ioctl(fd, VIDIOC_QBUF, &buf);
		
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glUseProgram(shader_program);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glfwSwapBuffers(window);
	}

	glDeleteBuffers(1, &vbo);
	glfwTerminate();
	return EXIT_SUCCESS;
}
