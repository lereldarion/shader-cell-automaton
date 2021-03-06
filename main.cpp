#include "main.h"

#include <cmath>

int main (int argc, char ** argv) {
	QGuiApplication app (argc, argv);

	GlWindow window (QSize (480, 480)); // Here is the size of the jacobi map (doesn't change with the window size)
	window.resize (480, 480);
	window.show ();

	window.setAnimating (true);

	return app.exec ();
}

GlWindow::GlWindow (const QSize & render_size) : OpenGLWindow (0, true),
	m_render_size (render_size),
	m_frame (0)
{
}

GlWindow::~GlWindow () {
}


void GlWindow::initialize () {
	initRJ ();
	initRS ();
	
	timer_init ();
}

void GlWindow::render () {
	renderRJ ();
	renderRS ();
	++m_frame;
}

/* timer stuff */
void GlWindow::timer_init (void) {
	connect (&m_timer, SIGNAL (timeout ()), this, SLOT (timer_end ()));
	m_timer.start (10000);
	m_frame_offset = 0;
}

void GlWindow::timer_end (void) {
	double fps_elapsed = static_cast< double > (m_frame - m_frame_offset) / 10.;
	qDebug () << "FPS" << fps_elapsed;
	m_frame_offset = m_frame;
}

/* Render Jacobi */

void GlWindow::initRJ (void) {
	// Shaders
	m_rj_program = new QOpenGLShaderProgram (this);
	m_rj_program->addShaderFromSourceFile (QOpenGLShader::Vertex, "wave.vertex.glsl");
	m_rj_program->addShaderFromSourceFile (QOpenGLShader::Fragment, "wave.fragment.glsl");
	m_rj_program->link ();

	m_rj_program->setUniformValue ("prev_jacobi", 0); // texture unit 0

	// Create a initial value map (may be changed)
	GLfloat img_data[m_render_size.height ()][m_render_size.width ()][2];

	for (int y = 0; y < m_render_size.height (); ++y) {
		for (int x = 0; x < m_render_size.width (); ++x) {
			int h = x - m_render_size.width () / 2;
			int v = y - m_render_size.height () / 2;
			img_data[y][x][0] = sinf(M_PI*h/48.0)*sinf(M_PI*v/48.0);
			img_data[y][x][1] = 0;
		}
	}

	// Init framebuffers
	for (int i = 0; i < 2; ++i) {
		m_rj_fbos[i] = new QOpenGLFramebufferObject (m_render_size,
				QOpenGLFramebufferObject::NoAttachment,
				GL_TEXTURE_2D, // standard 2D texture, will use texel*() to have absolute coords
				GL_RG32F); // one float32 per cell

		glBindTexture (GL_TEXTURE_2D, m_rj_fbos[i]->texture ());
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Put initial image
		glTexSubImage2D (GL_TEXTURE_2D, // target
				0, // lod (base)
				0, 0, m_render_size.width (), m_render_size.height (), // size
				GL_RG, GL_FLOAT, img_data);
	}
}

void GlWindow::renderRJ (void) {
	glViewport(0, 0, m_render_size.width (), m_render_size.height ());

	glClear (GL_COLOR_BUFFER_BIT);

	m_rj_program->bind ();

	// origin texture (on unit 0)
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, m_rj_fbos[m_frame % 2]->texture ());

	// set target texture
	m_rj_fbos[1 - m_frame % 2]->bind ();

	static const GLfloat vertices[] = {
		1.0f, 1.0f,
		1.0f, -1.0f,
		-1.0f, -1.0f,
		-1.0f, 1.0f
	};

	int vertex_loc = m_rj_program->attributeLocation ("vertex_coords");

	m_rj_program->setAttributeArray (vertex_loc, vertices, 2);

	m_rj_program->enableAttributeArray (vertex_loc);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	m_rj_program->disableAttributeArray (vertex_loc);

	m_rj_program->release();

	// reset target
	QOpenGLFramebufferObject::bindDefault ();
}

/* Render for screen */

void GlWindow::initRS (void) {
	// Create shaders
	m_rs_program = new QOpenGLShaderProgram (this);
	m_rs_program->addShaderFromSourceFile (QOpenGLShader::Vertex, "screen.vertex.glsl");
	m_rs_program->addShaderFromSourceFile (QOpenGLShader::Fragment, "screen.fragment.glsl");
	m_rs_program->link ();

	m_rs_program->setUniformValue ("jacobi_texture", 0); // Texture unit 0
}

void GlWindow::renderRS (void) {
	glViewport(0, 0, width (), height ());

	glClear (GL_COLOR_BUFFER_BIT);

	m_rs_program->bind ();

	// texture on unit 0	
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, m_rj_fbos[1 - m_frame % 2]->texture ());

	m_rs_program->setUniformValue ("width", (GLfloat) width ());
	m_rs_program->setUniformValue ("height", (GLfloat) height ());

	static const GLfloat vertices[] = {
		1.0f, 1.0f,
		1.0f, -1.0f,
		-1.0f, -1.0f,
		-1.0f, 1.0f
	};

	int vertex_loc = m_rs_program->attributeLocation ("vertex_coords");

	m_rs_program->setAttributeArray (vertex_loc, vertices, 2);

	m_rs_program->enableAttributeArray (vertex_loc);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	m_rs_program->disableAttributeArray (vertex_loc);

	m_rs_program->release();
}

