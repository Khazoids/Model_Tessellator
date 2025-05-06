#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cyTriMesh.h>
#include <cyVector.h>
#include <cyCore.h>
#include <cyGL.h>
#include <cyMatrix.h>
#include "lodepng.h"
#include "ClientState.h"
#include "ModelState.h"

#define WINDOW_HEIGHT 720
#define WINDOW_WIDTH 1080
#define PI 3.14159265358979323846
#define DEG2RAD(degrees) (degrees * PI / 180.0)

const int NUM_PATCH_PTS = 4;
const int MAX_TESSELLATION_LEVEL = 70;
const float MAX_DISPLACEMENT_FACTOR = 0.5;

float degrees = 45.0;
float displacementFactor = 0;
float tessellationLevel = NUM_PATCH_PTS;

GLuint QuadVAO = -1;
GLuint QuadVBO[4];

cyGLSLProgram mappingProgram, triangulationProgram;

// rotation, translation, scale
ModelState scene = ModelState(cyVec3f(0.0, 0.0, 0.0), cyVec3f(0, 0.0, 0.0), cyVec3f(1, 1, 1));
ClientState client;

// forward declarations
bool Init(char** argv);
void InitializeGlutCallBacks();
void CompileShaders(char** argv);
void CreateQuadVAO();
void LoadTexture(char const* file_path);
void RenderQuad();

cyVec3f ComputeTangent(cyVec3f p1, cyVec3f p2, cyVec3f p3, cyVec2f uv1, cyVec2f uv3, cyVec2f uv4);
cyMatrix4f GetModelViewProjection(cyVec3f translationXYZ, cyVec3f rotationXYZ, cyVec3f scaleXYZ);
cyMatrix4f GetModelViewTransformation(cyVec3f translation, cyVec3f rotation, cyVec3f scale);

// GLUT func declarations
void RenderSceneCB();
void MouseAction(int button, int state, int x, int y);
void KeyboardAction(unsigned char key, int x, int y);
void SpecialInput(int key, int x, int y);
void MouseMove(int x, int y);
void Idle();

// Main Execution Methods------------------------
int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "Please provide an argument to your normal map");
		exit(1);
	}
	glutInit(&argc, argv);

	// window initialization
	glutInitContextFlags(GLUT_DEBUG);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitWindowPosition(100, 100);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("Project 8");

	// GLEW initialization
	GLenum res = glewInit();
	if (res != GLEW_OK) {
		fprintf(stderr, "Error initializing GLEW.");
		exit(1);
	}

	CY_GL_REGISTER_DEBUG_CALLBACK;

	// callback registrations
	InitializeGlutCallBacks();

	if (!Init(argv))
	{
		return 1;
	}

	glutMainLoop();

	return 0;

}

bool Init(char** argv)
{
	CompileShaders(argv);
	CreateQuadVAO();

	return true;
}

void InitializeGlutCallBacks()
{
	glutDisplayFunc(RenderSceneCB);
	glutMouseFunc(MouseAction);
	glutMotionFunc(MouseMove);
	glutIdleFunc(Idle);
	glutKeyboardFunc(KeyboardAction);
	glutSpecialFunc(SpecialInput);
}

void CompileShaders(char** argv)
{
	GLint normal_map_unit = 0;
	GLint displacement_map_unit = 1;

	if (argv[2])
	{	
		// compile triangulation program with tessellation shaders attached
		if (!triangulationProgram.BuildFiles("shaders/triangulation.vert", "shaders/triangulation.frag", "shaders/triangulation.geo", "shaders/triangulation.tcs", "shaders/triangulation.tes"))
		{
			fprintf(stderr, "Error compiling triangulationProgram.");
			exit(1);
		}
		triangulationProgram.Bind();
		triangulationProgram.RegisterUniforms("mvp hasDisplacementMap displacementMap displacementFactor tessellationLevel");
		triangulationProgram.SetUniform(1, true);
		triangulationProgram.SetUniform(2, displacement_map_unit);

		

		// compile mapping program with tessellation shaders attached
		if (!mappingProgram.BuildFiles("shaders/displacement_map.vert", "shaders/displacement_map.frag", nullptr, "shaders/displacement_map.tcs", "shaders/displacement_map.tes"))
		{
			fprintf(stderr, "Error compiling displacementMapProgram.");
			exit(1);
		}

		mappingProgram.Bind();
		mappingProgram.RegisterUniforms("mvp mv normalMap displacementMap displacementFactor tessellationLevel");

		mappingProgram.SetUniform(3, displacement_map_unit);

		glActiveTexture(GL_TEXTURE0 + displacement_map_unit);
		LoadTexture(argv[2]);

		glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS);
	}
	else
	{	
		// compile triangulation program without tessellation shaders
		if (!triangulationProgram.BuildFiles("shaders/triangulation.vert", "shaders/triangulation.frag", "shaders/triangulation.geo"))
		{
			fprintf(stderr, "Error compiling triangulationProgram.");
			exit(1);
		}
		triangulationProgram.Bind();
		triangulationProgram.RegisterUniforms("mvp");

		// compile mapping program without tessellation shaders
		if (!mappingProgram.BuildFiles("shaders/normal_map.vert", "shaders/normal_map.frag"))
		{
			fprintf(stderr, "Error compiling normalMapProgram");
			exit(1);
		}

		mappingProgram.Bind();
		mappingProgram.RegisterUniforms("mvp mv normalMap");
	}

	// the normal map will alwayas be set for the mapping program regardless of the arguments provided.
	// we can set it right away outside of the if statements since we're already bound to the mapping program
	mappingProgram.SetUniform(2, normal_map_unit);
	glActiveTexture(GL_TEXTURE0 + normal_map_unit);
	LoadTexture(argv[1]);
}

void LoadTexture(char const* file_path)
{
	std::vector<unsigned char> image;
	unsigned width, height;
	unsigned error = lodepng::decode(image, width, height, file_path);

	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

	mappingProgram.Bind();

	GLuint texID;
	glGenTextures(1, &texID);

	glBindTexture(GL_TEXTURE_2D, texID);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		width,
		height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		&image[0]
	);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(
		GL_TEXTURE_2D,
		GL_TEXTURE_MIN_FILTER,
		GL_LINEAR_MIPMAP_LINEAR
	);
	glTexParameteri(
		GL_TEXTURE_2D,
		GL_TEXTURE_MAG_FILTER,
		GL_LINEAR
	);

	glTexParameteri(
		GL_TEXTURE_2D,
		GL_TEXTURE_WRAP_S,
		GL_REPEAT
	);
}


cyVec3f ComputeTangent(cyVec3f p1, cyVec3f p2, cyVec3f p3, cyVec2f uv1, cyVec2f uv2, cyVec2f uv3)
{
	cyVec3f edge1 = p2 - p1;
	cyVec3f edge2 = p3 - p1;
	cyVec2f deltaUV1 = uv2 - uv1;
	cyVec2f deltaUV2 = uv3 - uv1;


	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	cyVec3f tangent(
		f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
		f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
		f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z)
	);

	return tangent;
}

void CreateQuadVAO()
{
	glGenVertexArrays(1, &QuadVAO);
	glBindVertexArray(QuadVAO);
	glGenBuffers(4, QuadVBO);

	// positions
	cyVec3f pos1(-1.0f, 1.0f, 0.0f);
	cyVec3f pos2(-1.0f, -1.0f, 0.0f);
	cyVec3f pos3(1.0f, -1.0f, 0.0f);
	cyVec3f pos4(1.0f, 1.0f, 0.0f);

	// texture coordinates
	cyVec2f uv1(0.0f, 1.0f);
	cyVec2f uv2(0.0f, 0.0f);
	cyVec2f uv3(1.0f, 0.0f);
	cyVec2f uv4(1.0f, 1.0f);

	// normals
	cyVec3f nm(0.0f, 0.0f, 1.0f);

	// compute tangents for TBN matrix
	cyVec3f tangent1 = ComputeTangent(pos1, pos2, pos3, uv1, uv2, uv3);
	cyVec3f tangent2 = ComputeTangent(pos1, pos3, pos4, uv1, uv3, uv4);

	std::vector<cyVec3f> Vertices, Normals, Tangents;
	std::vector<cyVec2f> TexCoords;

	// check if we're using tessellation shaders
	if (glGetUniformLocation(mappingProgram.GetID(), "displacementFactor") != -1)
	{
		Vertices = { pos2, pos3, pos4, pos1 };
		Normals = { nm,nm,nm,nm };
		TexCoords = { uv2,uv3,uv4,uv1 };
		Tangents = { tangent1,tangent1,tangent2,tangent2 }; 
	}
	else
	{
		Vertices = { pos1, pos2, pos3, pos1, pos3, pos4 };
		Normals = { nm, nm, nm, nm, nm, nm };
		TexCoords = { uv1, uv2, uv3, uv1, uv3, uv4 };
		Tangents = { tangent1, tangent1, tangent1, tangent2, tangent2, tangent2 };
	}

	mappingProgram.Bind();

	// set attribute buffers
	mappingProgram.SetAttribBuffer("aPos", QuadVBO[0], 3);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f) * Vertices.size(), &Vertices[0], GL_STATIC_DRAW);

	mappingProgram.SetAttribBuffer("aNormals", QuadVBO[1], 3);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f) * Normals.size(), &Normals[0], GL_STATIC_DRAW);

	mappingProgram.SetAttribBuffer("aTexCoords", QuadVBO[2], 2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec2f) * TexCoords.size(), &TexCoords[0], GL_STATIC_DRAW);

	mappingProgram.SetAttribBuffer("aTangent", QuadVBO[3], 3);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f) * Tangents.size(), &Tangents[0], GL_STATIC_DRAW);

	// enable attribute buffers
	mappingProgram.EnableAttrib("aPos");
	mappingProgram.EnableAttrib("aNormals");
	mappingProgram.EnableAttrib("aTexCoords");
	mappingProgram.EnableAttrib("aTangent");

	triangulationProgram.Bind();

	triangulationProgram.SetAttribBuffer("aPos", QuadVBO[0], 3);
	triangulationProgram.EnableAttrib("aPos");

	// normals and tex are only set for triangulation program if it using tessellation shaders
	if (glGetUniformLocation(triangulationProgram.GetID(), "displacementMap") != -1)
	{
		triangulationProgram.SetAttribBuffer("aNormals", QuadVBO[1], 3);
		triangulationProgram.SetAttribBuffer("aTexCoords", QuadVBO[2], 2);
		triangulationProgram.EnableAttrib("aNormals");
		triangulationProgram.EnableAttrib("aTexCoords");
	}
	
	// unbind the VAO
	glBindVertexArray(0);

	// disable buffer locations
	triangulationProgram.DisableAttrib("aPos");

	if (glGetUniformLocation(triangulationProgram.GetID(), "displacementMap") != -1)
	{
		triangulationProgram.DisableAttrib("aNormals");
		triangulationProgram.DisableAttrib("aTexCoords");
	}

	mappingProgram.Bind();

	mappingProgram.DisableAttrib("aPos");
	mappingProgram.DisableAttrib("aNormals");
	mappingProgram.DisableAttrib("aTexCoords");
	mappingProgram.DisableAttrib("aTangent");
}


// GLUT Callbacks---------------------------------------

static void RenderSceneCB()
{
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	RenderQuad();
	glutSwapBuffers();
}

void KeyboardAction(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27: // esc
		glutLeaveMainLoop();
		break;
	case 32: //space
		client.spaceKeyToggled = !client.spaceKeyToggled;
		break;
	}
}

void SpecialInput(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		if (displacementFactor < MAX_DISPLACEMENT_FACTOR) displacementFactor += 0.01;
		break;
	case GLUT_KEY_DOWN:
		if (displacementFactor > 0) displacementFactor -= 0.01;
		break;
	case GLUT_KEY_RIGHT:
		if (tessellationLevel < MAX_TESSELLATION_LEVEL) tessellationLevel += 1;
		break;
	case GLUT_KEY_LEFT:
		if (tessellationLevel > NUM_PATCH_PTS) tessellationLevel -= 1;
		break;
	}
}

void MouseAction(int button, int state, int x, int y)
{
	client.x = x;
	client.y = y;
	if (GLUT_RIGHT_BUTTON == button) {
		client.rightMouseDown = !state;
		client.y = y;
	}
}

void MouseMove(int x, int y) {

	if (client.rightMouseDown) {
		if (degrees < 0.0f) degrees = 0.0f;
		else if (degrees > 180.0f) degrees = 180.0f;
		else degrees -= (.1 * (y - client.y));

		client.y = y;
	}
	else
	{
		scene.IncrementRotation((client.y - y) / WINDOW_HEIGHT * 5, (client.x - x) / WINDOW_WIDTH * 5, 0.0);
		client.x = x;
		client.y = y;
	}
}

void Idle()
{
	glutPostRedisplay();
}

void RenderQuad()
{
	glBindVertexArray(QuadVAO);
	mappingProgram.Bind();

	float quad_mvp_buffer[16];
	GetModelViewProjection(scene.translation, scene.rotation, scene.scale).Get(quad_mvp_buffer);
	mappingProgram.SetUniformMatrix4(0, quad_mvp_buffer);

	float quad_mv_buffer[16];
	GetModelViewTransformation(scene.translation, scene.rotation, scene.scale).Get(quad_mv_buffer);
	mappingProgram.SetUniformMatrix4(1, quad_mv_buffer);

	// check if we're using tessellation shaders
	if (glGetUniformLocation(mappingProgram.GetID(), "displacementFactor") != -1)
	{
		mappingProgram.SetUniform(4, displacementFactor);
		mappingProgram.SetUniform(5, tessellationLevel);
		glDrawArrays(GL_PATCHES, 0, NUM_PATCH_PTS);
	}
	else
	{
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	if (client.spaceKeyToggled)
	{
		triangulationProgram.Bind();
		triangulationProgram.SetUniformMatrix4(0, quad_mvp_buffer);

		if (glGetUniformLocation(mappingProgram.GetID(), "displacementMap") != -1)
		{
			triangulationProgram.SetUniform(3, displacementFactor);
			triangulationProgram.SetUniform(4, tessellationLevel);
			glDrawArrays(GL_PATCHES, 0, NUM_PATCH_PTS);
		}
		else
		{
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}

	glBindVertexArray(0);
}


cyMatrix4f GetModelViewProjection(cyVec3f translation, cyVec3f rotation, cyVec3f scale)
{
	cy::Matrix4f model = cy::Matrix4f::Scale(scale) * cy::Matrix4f::RotationXYZ(rotation.x, rotation.y, rotation.z) * cy::Matrix4f::Translation(translation);

	cy::Matrix4f view = cy::Matrix4f::View(
		cyVec3f(0.0, 0.0, 3.0),
		cyVec3f(0.0, 0.0, 0.0),
		cyVec3f(0.0, 1.0, 0.0)
	);

	cy::Matrix4f projection = cy::Matrix4f::Perspective(DEG2RAD(degrees), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 1000.0f);

	return projection * view * model;
}

cyMatrix4f GetModelViewTransformation(cyVec3f translation, cyVec3f rotation, cyVec3f scale)
{
	cy::Matrix4f model = cy::Matrix4f::Scale(scale) * cy::Matrix4f::RotationXYZ(rotation.x, rotation.y, rotation.z) * cy::Matrix4f::Translation(translation);

	cy::Matrix4f view = cy::Matrix4f::View(
		cyVec3f(0.0, 0.0, 3.0),
		cyVec3f(0.0, 0.0, 0.0),
		cyVec3f(0.0, 1.0, 0.0)
	);

	return view * model;
}

