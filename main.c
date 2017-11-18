#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include <gtk/gtk.h>
#include <math.h>
#include <glib.h>
#include "DashGL/dashgl.h"

static void on_realize(GtkGLArea *area);
static void on_render(GtkGLArea *area, GdkGLContext *context);
static gboolean on_idle(gpointer data);

#define WIDTH 640.0f
#define HEIGHT 480.0f

float timer;
GLuint vbo_cube_vertices;
GLuint ibo_cube_elements;
GLuint program, vao;
GLint attribute_coord3d, attribute_v_color;
GLint uniform_mvp;

int main(int argc, char *argv[]) {

	GtkWidget *window;
	GtkWidget *glArea;

	gtk_init(&argc, &argv);

	timer = 0.0f;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "GTK+ Cube");
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
	g_signal_connect(GTK_WINDOW(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	glArea = gtk_gl_area_new();
	gtk_widget_set_vexpand(glArea, TRUE);
	gtk_widget_set_hexpand(glArea, TRUE);
	g_signal_connect(glArea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glArea, "render", G_CALLBACK(on_render), NULL);
	gtk_container_add(GTK_CONTAINER(window), glArea);

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

static void on_realize(GtkGLArea *area) {

	gtk_gl_area_make_current(area);
	if(gtk_gl_area_get_error(area) != NULL) {
		fprintf(stderr, "Unknown error\n");
		return;
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	const char *vs = "shader/vertex.glsl";
	const char *fs = "shader/fragment.glsl";
	program = shader_load_program(vs, fs);

	GLfloat cube_vertices[] = {
		-1.0, -1.0,  1.0, 1.0, 0.0, 0.0,
		 1.0, -1.0,  1.0, 0.0, 1.0, 0.0,
		 1.0,  1.0,  1.0, 0.0, 0.0, 1.0,
		-1.0,  1.0,  1.0, 1.0, 1.0, 1.0,
		-1.0, -1.0, -1.0, 1.0, 0.0, 0.0,
		 1.0, -1.0, -1.0, 0.0, 1.0, 0.0,
		 1.0,  1.0, -1.0, 0.0, 0.0, 1.0,
		-1.0,  1.0, -1.0, 1.0, 1.0, 1.0
	};
	glGenBuffers(1, &vbo_cube_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

	GLushort cube_elements[] = {
    	// front
    	0, 1, 2,
    	2, 3, 0,
    	// top
    	1, 5, 6,
    	6, 2, 1,
    	// back
    	7, 6, 5,
    	5, 4, 7,
    	// bottom
    	4, 0, 3,
    	3, 7, 4,
    	// left
    	4, 5, 1,
    	1, 0, 4,
    	// right
    	3, 2, 6,
    	6, 7, 3,
	};
	glGenBuffers(1, &ibo_cube_elements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_elements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements, GL_STATIC_DRAW);

	const char* attribute_name;
	attribute_name = "coord3d";
	attribute_coord3d = glGetAttribLocation(program, attribute_name);
	if (attribute_coord3d == -1) {
		fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
		return;
	}

	attribute_name = "v_color";
	attribute_v_color = glGetAttribLocation(program, attribute_name);
	if (attribute_v_color == -1) {
		fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
		return;
	}

	const char* uniform_name;
	uniform_name = "mvp";
	uniform_mvp = glGetUniformLocation(program, uniform_name);
	if (uniform_mvp == -1) {
		fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
		return;
	}

	g_timeout_add(1000, on_idle, (void*)area);
}

static void on_render(GtkGLArea *area, GdkGLContext *context) {
	
	int size;
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glUseProgram(program);
	glEnableVertexAttribArray(attribute_coord3d);
	glEnableVertexAttribArray(attribute_v_color);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_vertices);
	
	glVertexAttribPointer(
		attribute_coord3d, 
		3,
		GL_FLOAT,
		GL_FALSE, 
		sizeof(float)*6,
		0
	);

	glVertexAttribPointer(
		attribute_v_color,
		3,
		GL_FLOAT,
		GL_FALSE,
		sizeof(float)*6,
		(void*)(sizeof(float)*3)
	);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_elements);
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	glDrawElements(GL_TRIANGLES, size/sizeof(GLushort), GL_UNSIGNED_SHORT, 0);

	glDisableVertexAttribArray(attribute_coord3d);
	glDisableVertexAttribArray(attribute_v_color);

}

static gboolean on_idle(gpointer data) {
	
	timer += 0.2f;
	float angle =  timer / 1000.0 * 45;
	float rad = angle * M_PI / 180.0;

	vec3 eye = { 0.0f, 2.0f, 0.0f };
	vec3 target = { 0.0f, 0.0f, -4.0f };
	vec3 axis = { 0.0f, 1.0f, 0.0f };
	vec3 t = { 0.0, 0.0, -4.0f };

	mat4 mvp, pos, rot, projection, view;
	mat4_identity(mvp);
	mat4_perspective(45.0f, 1.0f*WIDTH/HEIGHT, 0.1f, 10.0f, projection);
	mat4_lookat(eye, target, axis, view);
	mat4_translate(t, pos);
	mat4_rotate_y(rad, rot);

	mat4_multiply(mvp, projection, mvp);
	mat4_multiply(mvp, view, mvp);
	mat4_multiply(mvp, pos, mvp);
	mat4_multiply(mvp, rot, mvp);

	glUseProgram(program);
	glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, mvp);

	gtk_widget_queue_draw(GTK_WIDGET(data));
	return TRUE;

}
