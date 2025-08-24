///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the textures
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	// free the allocated OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"../../Utilities/textures/pages.jpg",
		"pages");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/page.jpg",
		"page");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/rubiks.jpg",
		"rubiks");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/shadow.jpg",
		"shadow");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}


/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/
	OBJECT_MATERIAL defaultMaterial;
	defaultMaterial.ambientColor = glm::vec3(0.8f, 0.8f, 0.8f);
	defaultMaterial.ambientStrength = 100.5f;
	defaultMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.8f);
	defaultMaterial.specularColor = glm::vec3(0.3f, 0.5f, 0.8f);
	defaultMaterial.shininess = 100.5;
	defaultMaterial.tag = "default_material";
	m_objectMaterials.push_back(defaultMaterial);

	OBJECT_MATERIAL tableMaterial;
	tableMaterial.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
	tableMaterial.ambientStrength = 1.0f;
	tableMaterial.diffuseColor = glm::vec3(0.8f, 0.7f, 0.8f);
	tableMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	tableMaterial.shininess = 1.1;
	tableMaterial.tag = "table_material";
	m_objectMaterials.push_back(tableMaterial);


	OBJECT_MATERIAL paperMaterial;
	paperMaterial.ambientColor = glm::vec3(0.99f, 0.99f, 0.99f);
	paperMaterial.ambientStrength = 0.99f;
	paperMaterial.diffuseColor = glm::vec3(0.99f, 0.99f, 0.99f);
	paperMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	paperMaterial.shininess = 100.0;
	paperMaterial.tag = "paper_material";
	m_objectMaterials.push_back(paperMaterial);

	OBJECT_MATERIAL wireMaterial;
	wireMaterial.ambientColor = glm::vec3(0.8f, 0.8f, 0.8f);
	wireMaterial.ambientStrength = 100.5f;
	wireMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.8f);
	wireMaterial.specularColor = glm::vec3(0.3f, 0.5f, 0.8f);
	wireMaterial.shininess = 100.5;
	wireMaterial.tag = "wire_material";
	m_objectMaterials.push_back(wireMaterial);

	OBJECT_MATERIAL rubiksMaterial;
	rubiksMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
	rubiksMaterial.ambientStrength = 1.0f;
	rubiksMaterial.diffuseColor = glm::vec3(0.9f, 0.5f, 0.5f);
	rubiksMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.9f);
	rubiksMaterial.shininess = 1.0;
	rubiksMaterial.tag = "rubiks_material";
	m_objectMaterials.push_back(rubiksMaterial);
}


/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/
	/*
	m_pShaderManager->setVec3Value("lightSources[0].position", -10.0, 0.1, 7.0);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.5f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.9f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.9f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 3.0f);

	m_pShaderManager->setVec3Value("lightSources[1].position", 10.0, 0.1, 7.0);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.0f, 0.0f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.0f, 0.0f, 0.9f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.0f, 0.0f, 0.9f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 30.0f);

	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0, 0.1, 8.0);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.0f, 0.5f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.0f, 0.9f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.0f, 0.9f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 3.0f);*/

	m_pShaderManager->setVec3Value("lightSources[0].position", 5.0, 4.0, -4.0);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.7f, 0.7f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.5f, 0.5f, 0.7f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 30.0f);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();

	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTorusMesh();
}



// dot product for the unit circle coordinates
void ucRot(float hUc, float vUc, float* hP, float* vP)
{
	float h = *vP * -vUc + *hP * hUc;
	float v = *hP * vUc + *vP * hUc;
	*hP = h;
	*vP = v;
}

// rotate position, rotation order is z, -(y), and then x
void rotate(float* xPos, float* yPos, float* zPos, float x, float y, float z)
{
	float xh, xv, yh, yv, zh, zv;

   z *= 3.14159 / 180.0;
   zh = cos(z);
   zv = sin(z);
   ucRot(zh, zv, xPos, yPos);
   
   y *= 3.14159 / 180.0;
   yh = cos(-y);
   yv = sin(-y);
   ucRot(yh, yv, xPos, zPos);

   x *= 3.14159 / 180.0;
   xh = cos(x);
   xv = sin(x);
   ucRot(xh, xv, yPos, zPos);
}


/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);

	SetShaderTexture("shadow");
	SetTextureUVScale(1.1, 1.1);
	SetShaderMaterial("table_material");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	SetShaderMaterial("default_material");

	int i;

	/****************************************************************/
	// draw pencil
	/****************************************************************/

	// pencil object rotation
	float pencil_xRot = 50.0;
	float pencil_yRot = 20.0;
	float pencil_zRot = 245.0;

	// pencil object position
	float pencil_x = 0.2;
	float pencil_y = 2.8;
	float pencil_z = 5.4;

	// cylinder dimensions for pencil
	float xSz1[] = { 0.3, 0.4, 0.25, 0.4, 0.075 };
	float ySz1[] = { 0.4, 0.6, 11.2, 10.8,  0.2 };
	float zSz1[] = { 0.3, 0.4, 0.25, 0.4, 0.075 };

	// cylinder rotations for pencil
	float xRot1[] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	float yRot1[] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	float zRot1[] = { 0.0, 0.0, 0.0, 0.0, 0.0 };

	// cylinder positions for pencil
	float xPos1[] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	float yPos1[] = { 0.0, 0.4, 1.0, 1.4, 14.8 };
	float zPos1[] = { 0.0, 0.0, 0.0, 0.0, 0.0 };

	// color
	float r1[] = { 0.9, 0.1, 0.1, 0.7, 0.1 };
	float g1[] = { 0.9, 0.1, 0.1, 0.7, 0.1 };
	float b1[] = { 0.9, 0.1, 0.1, 0.7, 0.1 };
	float a1[] = { 0.9, 0.9, 0.9, 0.5, 0.9 };

	// draw cylinders
	for (i = 0; i < 5; i++)
	{
		// set the XYZ scale
		scaleXYZ = glm::vec3(xSz1[i], ySz1[i], zSz1[i]);

		// set position, include orientation
		rotate(&xPos1[i], &yPos1[i], &zPos1[i], pencil_xRot, pencil_yRot, pencil_zRot);
		positionXYZ = glm::vec3(pencil_x + xPos1[i], pencil_y + yPos1[i], pencil_z + zPos1[i]);

		// set pencil rotation along with individual shape rotation
		XrotationDegrees = pencil_xRot + xRot1[i]; // limited use
		YrotationDegrees = pencil_yRot + yRot1[i];
		ZrotationDegrees = pencil_zRot + zRot1[i];

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		SetShaderColor(r1[i], g1[i], b1[i], a1[i]); // set the color values into the shader
		m_basicMeshes->DrawCylinderMesh(); // draw
	}


	// tapered cylinder dimensions for pencil
	float xSz2[] = { 0.4 };
	float ySz2[] = { 2.2 };
	float zSz2[] = { 0.4 };

	// tapered cylinder rotations for pencil
	float xRot2[] = { 0.0 };
	float yRot2[] = { 0.0 };
	float zRot2[] = { 0.0 };

	// tapered cylinder positions for pencil
	float xPos2[] = { 0.0 };
	float yPos2[] = { 12.2 };
	float zPos2[] = { 0.0 };

	// set the XYZ scale
	scaleXYZ = glm::vec3(xSz2[0], ySz2[0], zSz2[0]);

	// set position, include orientation
	rotate(&xPos2[0], &yPos2[0], &zPos2[0], pencil_xRot, pencil_yRot, pencil_zRot);
	positionXYZ = glm::vec3(pencil_x + xPos2[0], pencil_y + yPos2[0], pencil_z + zPos2[0]);

	// set pencil rotation along with individual shape rotation
	XrotationDegrees = pencil_xRot + xRot2[0]; // limited use
	YrotationDegrees = pencil_yRot + yRot2[0];
	ZrotationDegrees = pencil_zRot + zRot2[0];

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 0.9); // set the color values into the shader

	// draw tapered cylinder
	m_basicMeshes->DrawTaperedCylinderMesh(); // draw



	// box dimensions for pencil clip
	float xSz3[] = { 0.45, 0.4 };
	float ySz3[] = { 0.9, 3.4 };
	float zSz3[] = { 0.3, 0.12 };

	// box rotations for pencil clip
	float xRot3[] = { 0.0, 0.0 };
	float yRot3[] = { 0.0, 0.0 };
	float zRot3[] = { 0.0, 0.0 };

	// box positions for pencil clip
	float xPos3[] = { 0.0, 0.0 };
	float yPos3[] = { 2.25, 2.2 };
	float zPos3[] = { 0.4, 0.6 };

	// draw boxes
	for (i = 0; i < 2; i++)
	{
		// compensate for shape center offset
		yPos3[i] += ySz3[i] / 2;

		// set the XYZ scale
		scaleXYZ = glm::vec3(xSz3[i], ySz3[i], zSz3[i]);

		// set position, include orientation
		rotate(&xPos3[i], &yPos3[i], &zPos3[i], pencil_xRot, pencil_yRot, pencil_zRot);
		positionXYZ = glm::vec3(pencil_x + xPos3[i], pencil_y + yPos3[i], pencil_z + zPos3[i]);

		// set pencil rotation along with individual shape rotation
		XrotationDegrees = pencil_xRot + xRot3[i]; // limited use
		YrotationDegrees = pencil_yRot + yRot3[i];
		ZrotationDegrees = pencil_zRot + zRot3[i];

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		SetShaderColor(1, 0.4, 0.1, 0.9); // set the color values into the shader
		m_basicMeshes->DrawBoxMesh(); // draw
	}

	// sphere dimensions for pencil clip
	float xSz4[] = { 0.2 };
	float ySz4[] = { 0.2 };
	float zSz4[] = { 0.1 };

	// sphere rotations for pencil clip
	float xRot4[] = { 0.0 };
	float yRot4[] = { 0.0 };
	float zRot4[] = { 0.0 };

	// sphere positions for pencil clip
	float xPos4[] = { 0.0 };
	float yPos4[] = { 5.3 };
	float zPos4[] = { 0.52 };

	// set the XYZ scale
	scaleXYZ = glm::vec3(xSz4[0], ySz4[0], zSz4[0]);

	// set position, include orientation
	rotate(&xPos4[0], &yPos4[0], &zPos4[0], pencil_xRot, pencil_yRot, pencil_zRot);
	positionXYZ = glm::vec3(pencil_x + xPos4[0], pencil_y + yPos4[0], pencil_z + zPos4[0]);

	// set pencil rotation along with individual shape rotation
	XrotationDegrees = pencil_xRot + xRot4[0]; // limited use
	YrotationDegrees = pencil_yRot + yRot4[0];
	ZrotationDegrees = pencil_zRot + zRot4[0];

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderColor(1, 0.4, 0.1, 0.7); // set the color values into the shader

	// draw sphere
	m_basicMeshes->DrawSphereMesh(); // draw


	// cone dimensions for pencil point
	float xSz5[] = { 0.2 };
	float ySz5[] = { 0.6 };
	float zSz5[] = { 0.2 };

	// cone rotations for pencil point
	float xRot5[] = { 0.0 };
	float yRot5[] = { 0.0 };
	float zRot5[] = { 0.0 };

	// cone positions for pencil point
	float xPos5[] = { 0.0 };
	float yPos5[] = { 14.4 };
	float zPos5[] = { 0.0 };

	// set the XYZ scale
	scaleXYZ = glm::vec3(xSz5[0], ySz5[0], zSz5[0]);

	// set position, include orientation
	rotate(&xPos5[0], &yPos5[0], &zPos5[0], pencil_xRot, pencil_yRot, pencil_zRot);
	positionXYZ = glm::vec3(pencil_x + xPos5[0], pencil_y + yPos5[0], pencil_z + zPos5[0]);

	// set pencil rotation along with individual shape rotation
	XrotationDegrees = pencil_xRot + xRot5[0]; // limited use
	YrotationDegrees = pencil_yRot + yRot5[0];
	ZrotationDegrees = pencil_zRot + zRot5[0];

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 0.9); // set the color values into the shader

	// draw cone
	m_basicMeshes->DrawConeMesh(); // draw

	/****************************************************************/
	// end of draw pencil
	/****************************************************************/


	/****************************************************************/
	// draw notebook
	/****************************************************************/

	// notebook object rotation
	float notebook_xRot = 0.0;
	float notebook_yRot = 5.0;
	float notebook_zRot = 0.0;

	// notebook object position
	float notebook_x = 5.5;
	float notebook_y = 0.0;
	float notebook_z = 0.0;

	// box dimensions for notebook
	float xSz6[] = { 10.0 };
	float ySz6[] = { 2.0 };
	float zSz6[] = { 14.0 };

	// box rotations for notebook
	float xRot6[] = { 0.0 };
	float yRot6[] = { 0.0 };
	float zRot6[] = { 0.0 };

	// box positions for notebook
	float xPos6[] = { 0.0 };
	float yPos6[] = { 0.0 };
	float zPos6[] = { 0.0 };

	// compensate for shape center offset
	yPos6[0] += ySz6[0] / 2;

	// set the XYZ scale
	scaleXYZ = glm::vec3(xSz6[0], ySz6[0], zSz6[0]);

	// set position, include orientation
	rotate(&xPos6[0], &yPos6[0], &zPos6[0], notebook_xRot, notebook_yRot, notebook_zRot);
	positionXYZ = glm::vec3(notebook_x + xPos6[0], notebook_y + yPos6[0], notebook_z + zPos6[0]);

	// set notebook rotation along with individual shape rotation
	XrotationDegrees = notebook_xRot + xRot6[0]; // limited use
	YrotationDegrees = notebook_yRot + yRot6[0];
	ZrotationDegrees = notebook_zRot + zRot6[0];

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderColor(1, 1, 1, 1); // set the color values into the shader

	SetShaderTexture("pages");
	SetTextureUVScale(1.0, 1.0);

	// draw box
	m_basicMeshes->DrawBoxMesh(); // draw


	// plane dimensions for page
	float xSz7[] = { 5.0 };
	float ySz7[] = { 1.0 };
	float zSz7[] = { 7.0 };

	// plane rotations for page
	float xRot7[] = { 0.0 };
	float yRot7[] = { -1.0 };
	float zRot7[] = { 0.0 };

	// plane positions for page
	float xPos7[] = { 0.1 };
	float yPos7[] = { 2.02 };
	float zPos7[] = { 0.0 };

	// set the XYZ scale
	scaleXYZ = glm::vec3(xSz7[0], ySz7[0], zSz7[0]);

	// set position, include orientation
	rotate(&xPos7[0], &yPos7[0], &zPos7[0], notebook_xRot, notebook_yRot, notebook_zRot);
	positionXYZ = glm::vec3(notebook_x + xPos7[0], notebook_y + yPos7[0], notebook_z + zPos7[0]);

	// set notebook rotation along with individual shape rotation
	XrotationDegrees = notebook_xRot + xRot7[0]; // limited use
	YrotationDegrees = notebook_yRot + yRot7[0];
	ZrotationDegrees = notebook_zRot + zRot7[0];

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderColor(1, 1, 1, 1); // set the color values into the shader

	SetShaderTexture("page");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("paper_material");

	//draw plane
	m_basicMeshes->DrawPlaneMesh();

	SetShaderMaterial("default_material");

	// ring dimensions for notebook
	float xSz8[17];
	float ySz8[17];
	float zSz8[17];

	// ring rotations for notebook
	float xRot8[17];
	float yRot8[17];
	float zRot8[17];

	// ring positions for notebook
	float xPos8[17];
	float yPos8[17];
	float zPos8[17];

	// draw rings
	for (i = 0; i < 17; i++)
	{
		xSz8[i] = 0.25;
		ySz8[i] = 0.25;
		zSz8[i] = 0.25;
		xRot8[i] = 0.0;
		yRot8[i] = 0.0;
		zRot8[i] = 0.0;
		xPos8[i] = -5.0;
		yPos8[i] = 1.0;
		zPos8[i] = 13.5 / 17 * (8 - i); // place each ring

		// compensate for shape center offset
		yPos8[i] += ySz8[i] / 2;

		// set the XYZ scale
		scaleXYZ = glm::vec3(xSz8[i], ySz8[i], zSz8[i]);

		// set position, include orientation
		rotate(&xPos8[i], &yPos8[i], &zPos8[i], notebook_xRot, notebook_yRot, notebook_zRot);
		positionXYZ = glm::vec3(notebook_x + xPos8[i], notebook_y + yPos8[i], notebook_z + zPos8[i]);

		// set notebook rotation along with individual shape rotation
		XrotationDegrees = notebook_xRot + xRot8[i]; // limited use
		YrotationDegrees = notebook_yRot + yRot8[i];
		ZrotationDegrees = notebook_zRot + zRot8[i];

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		SetShaderColor(0.7, 0.7, 0.7, 0.9); // set the color values into the shader
		m_basicMeshes->DrawTorusMesh(); // draw
	}
	/****************************************************************/
	// end of draw notebook
	/****************************************************************/


	/****************************************************************/
	// draw rubik's cubes
	/****************************************************************/

	// rubiks object rotation
	float rubiks_xRot = 0.0;
	float rubiks_yRot = 0.0;
	float rubiks_zRot = 0.0;

	// rubiks object position
	float rubiks_x = -5.5;
	float rubiks_y = 0.0;
	float rubiks_z = 0.0;

	// box dimensions for rubiks
	float xSz9[] = { 3.0, 3.0, 3.0, 3.0 };
	float ySz9[] = { 3.0, 3.0, 3.0, 3.0 };
	float zSz9[] = { 3.0, 3.0, 3.0, 3.0 };

	// box rotations for rubiks
	float xRot9[] = { 0.0, 180.0, 0.0, 90.0 };
	float yRot9[] = { 0.0, 0.0, -90.0, 180.0 };//-45
	float zRot9[] = { -90.0, 0.0, 0.0, 135.0 };

	// box positions for rubiks
	float xPos9[] = { 0.0, -3.0, -3.0, -1.5 };
	float yPos9[] = { 0.0, 0.0, 0.0, 3.0 };
	float zPos9[] = { 0.0, 1.5, -1.5, 0.0 };

	// draw rings
	for (i = 0; i < 4; i++)
	{
		// compensate for shape center offset
		yPos9[i] += ySz9[i] / 2;

		// set the XYZ scale
		scaleXYZ = glm::vec3(xSz9[i], ySz9[i], zSz9[i]);

		// set position, include orientation
		rotate(&xPos9[i], &yPos9[i], &zPos9[i], rubiks_xRot, rubiks_yRot, rubiks_zRot);
		positionXYZ = glm::vec3(rubiks_x + xPos9[i], rubiks_y + yPos9[i], rubiks_z + zPos9[i]);

		// set rubiks rotation along with individual shape rotation
		XrotationDegrees = rubiks_xRot + xRot9[i]; // limited use
		YrotationDegrees = rubiks_yRot + yRot9[i];
		ZrotationDegrees = rubiks_zRot + zRot9[i];

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		SetShaderColor(1, 1, 1, 1); // set the color values into the shader

		SetShaderTexture("rubiks");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("rubiks_material");

		// draw box
		m_basicMeshes->DrawBoxMesh(); // draw
	}

	/****************************************************************/
	// end of rubik's cubes
	/****************************************************************/
}
