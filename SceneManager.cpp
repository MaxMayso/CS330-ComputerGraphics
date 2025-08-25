///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
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

// declaration of global variables and defines
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
	// create the shape meshes object
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
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
	// free the allocated objects
	m_pShaderManager = NULL;
	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}

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

	modelView = translation * rotationZ * rotationY * rotationX * scale;

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
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}


//-------------------------------------------------------------------------------------------------------------------MODIFY CODE BELOW-------------------------------------------
// _________________________________________________________ADDED CODE BY STUDENT - MAX _______________________________________________________THIS IS HERE FOR DETECTABILITY-----


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
	OBJECT_MATERIAL whiteMat;
	whiteMat.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	whiteMat.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	whiteMat.shininess = 32.0f;
	whiteMat.tag = "default";

	m_objectMaterials.push_back(whiteMat);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Disable unused lights first
	m_pShaderManager->setBoolValue("spotLight.bActive", false);
	for (int i = 0; i < 4; ++i)
		m_pShaderManager->setBoolValue(("pointLights[" + std::to_string(i) + "].bActive").c_str(), false);

	// Bright directional light to simulate daylight from the kitchen window
	m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(-0.3f, -1.0f, -0.3f));
	m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.2f, 0.2f, 0.2f));    // was 0.4
	m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));    // was 0.9
	m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(0.7f, 0.7f, 0.7f));   // was 1.0
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Warm overhead point light to soften the shadows, pancakes must look stacked
	m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(0.0f, 5.0f, 1.0f));
	m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.1f, 0.09f, 0.08f));    // was 0.2
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.6f, 0.5f, 0.4f));      // was 0.8
	m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(0.4f, 0.3f, 0.2f));     // was 0.9
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

}


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
		"textures/marble.png",
		"marbleFloor");

	bReturn = CreateGLTexture(
		"textures/berry.jpg",
		"berry");

	bReturn = CreateGLTexture(
		"textures/pancake_face.jpg",
		"pancakeFace");

	bReturn = CreateGLTexture(
		"textures/brickWall.png",
		"brick");

	//Bind to GPU
	BindGLTextures();

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

	//Call textures for loading
	LoadSceneTextures();
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadPyramid3Mesh();

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
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

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

	SetShaderTexture("marbleFloor");

	//Marble counter is tiled
	SetTextureUVScale(5.0f, 5.0f);
	SetShaderMaterial("default");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	//Start construction of backwall item
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 5.0f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("brick");

	//Brick wall scaling
	SetTextureUVScale(2.0f, 2.0f);
	//Set material required for shading
	SetShaderMaterial("default");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/


	// ======================= PLATE BASE ==================================

	glm::vec3 plateScale = glm::vec3(7.0f, 0.2f, 7.0f); // Wide and thin
	glm::vec3 platePosition = glm::vec3(0.0f, 0.1f, 0.0f); // Just above floor
	SetTransformations(plateScale, 0.0f, 0.0f, 0.0f, platePosition);
	// Optional: Use a white glossy ceramic color
	SetShaderColor(0.90f, 0.90f, 0.90f, 1.0f); // near-white
	//Set material required for shading
	SetShaderMaterial("default");
	m_basicMeshes->DrawTaperedCylinderMesh(); // Plate body

	// Function to create stack of pancake objects - Makes a pancake with a cylindermesh, then adds torus to end to make the pancake shaped properly
	for (int i = 0; i < 6; ++i) { // Number of pancakes = i
		float yHeight = 0.3f + (i * 0.35f);  //Height by the number of pancakes
		glm::vec3 scaleXYZ = glm::vec3(5.0f, 0.3f, 5.0f); // XYZ Scale to make all pancakes wide and flat
		glm::vec3 positionXYZ = glm::vec3(0.0f, yHeight, 0.0f);

		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ); //No XYZ rotationdegress needed

		//SetShaderColor(0.76f, 0.48f, 0.28f, 1);  // light brown pancake
		SetShaderTexture("pancakeFace");
		SetTextureUVScale(1.0f, 1.0f);
		
			//Set material required for shading
		SetShaderMaterial("default");
		m_basicMeshes->DrawCylinderMesh(); //Draws the Cylinder to the values, above then continues to the torus shape below

		glm::vec3 ringScale = glm::vec3(4.4f, 4.4f, 0.9f);  // Scaled all the same, wife and flat with slight curve on the depth for thickness
		glm::vec3 ringPosition = glm::vec3(0.0f, yHeight, 0.0f);  // Applied with the number of pancakes, on each pancake
		XrotationDegrees = 90.0f; //Applied to lay down torus flat
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		SetTransformations(ringScale, 
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees, 
			ringPosition);

		//SetShaderColor(0.76f, 0.52f, 0.28f, 1);  // slightly lighter tint of brown to resemble pancake coloring
		SetShaderTexture("pancakeFace");
		//Set material required for shading
		SetShaderMaterial("default");
		SetTextureUVScale(1.0f, yHeight);

		m_basicMeshes->DrawTorusMesh(); //Draws the torus and continues loop to next pancake

	}

	// ====================== ORANGE JUICE (Tapered Cylinder) ========================

	glm::vec3 glassScale = glm::vec3(1.2f, 2.8f, 1.2f); // tall and narrow
	glm::vec3 glassPosition = glm::vec3(8.0f, 3.0f, 0.0f); // just to the right of pancakes
	SetTransformations(glassScale, 180.0f, 0.0f, 0.0f, glassPosition);

	// Set glassy color (light blue-gray) and lower specular shine
	SetShaderColor(1.0f, 0.65f, 0.0f, 1.0f); // true orange, semi-transparent
	SetShaderMaterial("default");
	m_basicMeshes->DrawTaperedCylinderMesh();


	// ====================== GLASS BASE (Tapered Cylinder) ========================

	glm::vec3 juiceScale = glm::vec3(1.4f, 3.0f, 1.4f); // slightly smaller than glass
	glm::vec3 juicePosition = glm::vec3(8.0f, 3.25f, 0.0f); // inside the cup
	SetTransformations(juiceScale, 180.0f, 0.0f, 0.0f, juicePosition);

	// Set orange color (can also use a texture if you want)
	SetShaderColor(0.9f, 0.9f, 1.0f, 0.2f);  // light bluish glass with high transparency
	SetShaderMaterial("glass"); // use basic speculars
	m_basicMeshes->DrawTaperedCylinderMesh();

	// ---------------------BERRIES ----------------------------------BERRIES -----------------------------------BERRIES ----------------------------
	
	// ====================== EMPTY CLEAR BOTTLE ========================
	SetShaderColor(0.9f, 0.9f, 1.0f, 0.2f);  // light bluish glass with high transparency
	SetShaderMaterial("glass"); // use basic speculars

	// ---- Bottom Half Sphere (base of bottle) ----
	SetTransformations(
		glm::vec3(0.9f, 0.3f, 0.9f),
		0.0f, 0.0f, 180.0f,
		glm::vec3(6.0f, 0.9f, -1.8f));  // shifted back near the glass
	m_basicMeshes->DrawHalfSphereMesh();

	// ---- Main Cylinder (body of bottle) ----
	SetTransformations(
		glm::vec3(0.9f, 4.0f, 0.9f),
		0.0f, 0.0f, 0.0f,
		glm::vec3(6.0f, 0.9f, -1.8f));
	m_basicMeshes->DrawCylinderMesh();

	// ---- Rounded Top Half Sphere ----
	SetTransformations(
		glm::vec3(0.905f, 0.9f, 0.905f),
		0.0f, -6.0f, 0.0f,
		glm::vec3(6.0f, 4.9f, -1.8f));
	m_basicMeshes->DrawHalfSphereMesh();

	// ---- Top Neck Cylinder ----
	SetTransformations(
		glm::vec3(0.3f, 2.0f, 0.3f),
		0.0f, 0.0f, 0.0f,
		glm::vec3(6.0f, 5.6f, -1.8f));
	m_basicMeshes->DrawCylinderMesh();

	// ---- Cap Ring (Torus) ----
	SetTransformations(
		glm::vec3(0.32f, 0.32f, 1.5f),
		90.0f, 0.0f, 0.0f,
		glm::vec3(6.0f, 7.4f, -1.8f));
	m_basicMeshes->DrawTorusMesh();

	// ---- Bottle Rim (Torus) ----
	SetTransformations(
		glm::vec3(0.28f, 0.28f, 0.4f),
		90.0f, 0.0f, 0.0f,
		glm::vec3(6.0f, 7.6f, -1.8f));
	m_basicMeshes->DrawTorusMesh();

	//==========================Syrup==============================

	SetShaderColor(0.35f, 0.15f, 0.05f, 1.0f); // Molasses colored Or syrup
	SetShaderMaterial("default");

	// ---- Bottom Half Sphere (base of bottle) ----
	SetTransformations(
		glm::vec3(.91f, 2.7f, .91f),
		0.0f, 0.0f, 180.0f,
		glm::vec3(6.0f, 2.9f, -1.8f));  // shifted back near the glass cup
	m_basicMeshes->DrawCylinderMesh();

}
