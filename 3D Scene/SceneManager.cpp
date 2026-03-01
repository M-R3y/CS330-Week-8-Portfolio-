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
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
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
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	// *** For flexibility all available textures are written into the code here. 
	// *** Must keep in mind only 16 can be utilized for the scene at one time.
	// *** To keep things simple textures which will not be used can be commented out and when needed they can be uncommented.

	bool bReturn = false;

	//-----abstract-----
	bReturn = CreateGLTexture(
		"../../Utilities/textures/abstract.jpg",
		"1_abstract");

	////-----backdrop-----
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/backdrop.jpg",
	//	"2_backdrop");

	////-----breadcrust-----
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/breadcrust.jpg",
	//	"3_breadcrust");

	////-----cheddar-----
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/cheddar.jpg",
	//	"4_cheddar");

	////-----cheese_top-----
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/cheese_top.jpg",
	//	"5_cheese_top");

	////-----cheese_wheel-----
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/cheese_wheel.jpg",
	//	"6_cheese_wheel");

	////-----circular-brushed-gold-texture-----
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/circular-brushed-gold-texture.jpg",
	//	"7_circular-brushed-gold-texture");

	//-----drywall-----
	bReturn = CreateGLTexture(
		"../../Utilities/textures/drywall.jpg",
		"8_drywall");

	////-----gold-seamless-texture-----
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/gold-seamless-texture.jpg",
	//	"9_gold-seamless-texture");

	////-----knife_handle-----
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/knife_handle.jpg",
	//	"10_knife_handle");

	////-----pavers-----
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/pavers.jpg",
	//	"11_pavers");

	////-----rusticwood-----
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/rusticwood.jpg",
	//	"12_rusticwood");

	////-----stainglass-----
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/stainedglass.jpg",
	//	"13_stainedglass");

	//-----stainless-----
	bReturn = CreateGLTexture(
		"../../Utilities/textures/stainless.jpg",
		"14_stainless");

	////-----stainless_end-----
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/stainless_end.jpg",
	//	"15_stainless_end");

	////-----tiles2-----
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/tilesf2.jpg",
	//	"16_tiles2");

	////-----The_Wall_-_Free_For_Commercial_Use_-_FFCU_(26505175760)-----this image was sourced and added to utilize a brick texture-----
	////credits for this image are in CS330Content\Projects\Utilities\textures\Image Credits.txt
	//bReturn = CreateGLTexture(
	//	"../../Utilities/textures/The_Wall_-_Free_For_Commercial_Use_-_FFCU_(26505175760).jpg",
	//	"17_bricks");

	//-----tiles2-----
	bReturn = CreateGLTexture(
		"../../Utilities/textures/wood-texture-1437843675YVi.jpg",
		"18_wood_texture");

	//-----red background-----
	bReturn = CreateGLTexture(
		"../../Utilities/textures/red-background-14385385380R5.jpg",
		"19_red_background");

	//-----green paper-----
	bReturn = CreateGLTexture(
		"../../Utilities/textures/green-texture-paper.jpg",
		"20_green_textured_paper");

	//-----laptop plastic texture-----
	bReturn = CreateGLTexture(
		"../../Utilities/textures/images.jpg",
		"21_laptop_plastic");

	//////////////////bReturn = CreateGLTexture(
	//////////////////	"../../Utilities/textures/d78b6fb-3bf8314b-e256-403f-9c21-3f3a2e1c3a59.jpg",
	//////////////////	"22_laptop_screen");

	//-----laptop plastic texture-----
	bReturn = CreateGLTexture(
		"../../Utilities/textures/balsa_wood.jpg",
		"22_light_wood");


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

	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
	goldMaterial.ambientStrength = 0.4f;
	goldMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "gold";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cementMaterial.ambientStrength = 0.2f;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";

	m_objectMaterials.push_back(cementMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL tileMaterial;
	tileMaterial.ambientColor = glm::vec3(0.2f, 0.3f, 0.4f);
	tileMaterial.ambientStrength = 0.3f;
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	tileMaterial.shininess = 25.0;
	tileMaterial.tag = "tile";

	m_objectMaterials.push_back(tileMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f);
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 0.5;
	clayMaterial.tag = "clay";

	m_objectMaterials.push_back(clayMaterial);

	//metal material definition for the spray can bod
	OBJECT_MATERIAL steelMaterial;
	steelMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.28f);   		// neutral gray color
	steelMaterial.ambientStrength = 0.2f;
	steelMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.45f);     		// slightly gray
	steelMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.85f);  		// strong reflection
	steelMaterial.shininess = 64.0f;                               		// sharp highlights
	steelMaterial.tag = "steel";

	m_objectMaterials.push_back(steelMaterial);

	//plastic material definition for spray can nozzel
	OBJECT_MATERIAL whitePlastic;
	whitePlastic.ambientColor = glm::vec3(0.9f, 0.9f, 0.9f);   // almost white color
	whitePlastic.ambientStrength = 0.3f;
	whitePlastic.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);   // white base
	whitePlastic.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);  // soft highlight
	whitePlastic.shininess = 32.0f;                            // smooth plastic
	whitePlastic.tag = "whitePlastic";

	m_objectMaterials.push_back(whitePlastic);

	// laptop screen material high sheen
	OBJECT_MATERIAL screenMaterial;
	screenMaterial.ambientColor = glm::vec3(0.02f, 0.02f, 0.03f); 
	screenMaterial.ambientStrength = 0.1f;                        
	screenMaterial.diffuseColor = glm::vec3(0.05f, 0.05f, 0.07f); 
	screenMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);    
	screenMaterial.shininess = 64.0f;                          
	screenMaterial.tag = "laptopScreen";

	m_objectMaterials.push_back(screenMaterial);

	//balsa wood material definition softer specular for lower sheen and slightly muted for a cleaner wood material
	OBJECT_MATERIAL balsaWood;
	balsaWood.ambientColor = glm::vec3(0.18f, 0.16f, 0.11f);   
	balsaWood.ambientStrength = 0.18f;                         
	balsaWood.diffuseColor = glm::vec3(0.55f, 0.50f, 0.36f);   
	balsaWood.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);     
	balsaWood.shininess = 6.0f;                                
	balsaWood.tag = "balsaWood";

	m_objectMaterials.push_back(balsaWood);

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
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	// Light 0
	m_pShaderManager->setVec3Value("lightSources[0].position", 3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	// Light 1
	m_pShaderManager->setVec3Value("lightSources[1].position", -3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);

	// Light 2
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.6f, 5.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.5f);

	// Light 3
	// lowering all of the strength values of this like so it faced the front of the scene but doesn't overpower it
	// also placing it far away from the scene in the X axis to keep it from overpowering scene
	m_pShaderManager->setVec3Value("lightSources[3].position", 60.0f, 1.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 1e-4f, 1e-4f, 1e-4f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 1e-4f, 1e-4f, 1e-4f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 1e-4f, 1e-4f, 1e-4f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 30.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 1e-4f);

	m_pShaderManager->setBoolValue("bUseLighting", true);

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
	// load the textures for the 3D scene ----important to load the scene textures before loading meshes 
	// loading textures before meshes allows them to be bound to the meshes when SetShaderTexture() is called at render
	LoadSceneTextures();

	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();		// Loading a cylinder mesh 
	m_basicMeshes->LoadSphereMesh();		// Loading a sphere mesh
	m_basicMeshes->LoadBoxMesh();

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
	YrotationDegrees = 90.0f;		//rotating floor plane so grain of the wood surface looks accurate
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

	//SetShaderColor(0.65f, 0.45f, 0.25f, 1.0f);		//Setting floor plane to a brown color to represent a table
	SetShaderTexture("18_wood_texture");				// setting a wooden texture for the floor
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


//***********************************************----- SPRAY CAN -----*************************************************************************
//-------------------------------------SPRAY CAN BODY BASIC SHAPES--------------------------------------------------------

/************************* --Spray Can Body-- ***************************************/
/*** Set needed transformations before drawing the basic mesh.  ***/
/*** This same ordering of code should be used for transforming ***/
/*** and drawing all the basic 3D shapes.						***/

/******************************************************************/
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 6.0f, 1.5f);		// Scaling cylinder body

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 0.0f, -3.0f);		

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.75f, 0.75f, 0.78f, 1.0f);		// Setting the cylinder to a grey color
	SetShaderTexture("14_stainless");					//setting a stainless texture
	SetShaderMaterial("steel");


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/************************ End of Spray Can Body ****************************************/


/************************* --Spray Sphere Top Transition up to nozzel-- ***************************************/
/*** Set needed transformations before drawing the basic mesh.  ***/
/*** This same ordering of code should be used for transforming ***/
/*** and drawing all the basic 3D shapes.						***/

/******************************************************************/
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.35f, 1.35f, 1.35f);		// Scaling sphere to be smalller than spray can body for transition up to nozzel

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 6.0f, -3.0f);		// setting sphere to so it sits halfway inside cylinder body

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.75f, 0.75f, 0.78f, 1.0f);		// Setting the sphere to a grey color
	SetShaderTexture("14_stainless");					//setting a stainless texture

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/************************ Spray Sphere Top Transition up to nozzel ****************************************/


/************************* --Spray Can Nozzel-- ***************************************/
/*** Set needed transformations before drawing the basic mesh.  ***/
/*** This same ordering of code should be used for transforming ***/
/*** and drawing all the basic 3D shapes.						***/

/******************************************************************/
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.5f, 0.3f);		// scaling nozzel to be a small cylinder

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 7.2f, -3.0f);		// positioning nozzel slightly inside sphere transition

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);		// Setting nozzel to white
	SetShaderTexture("8_drywall");				//setting the nozzel to the drywall texture so it adds a realistic effect 
	SetShaderMaterial("whitePlastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/************************ End of Spray Can Spray Can Nozzel ****************************************/


/************************* --Spray Can Body Spray Straw-- ***************************************/
/*** Set needed transformations before drawing the basic mesh.  ***/
/*** This same ordering of code should be used for transforming ***/
/*** and drawing all the basic 3D shapes.						***/

/******************************************************************/
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.06f, 4.0f, 0.06f);		// Scaling srapy straw to be thin but within the cylinder body

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -5.0f;		//slightly rotating spray straw

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.5f, 1.2f, -1.5f);		// positioning spray stray on front part of cylinder 

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.9f, 0.15f, 0.15f, 1.0f);		// Setting the spray straw to RED
	SetShaderTexture("19_red_background");			// setting a shiny red texture to the spray straw

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/************************ End of Spray Can Body ****************************************/

//------------------------------------- END OF SPRAY CAN BODY BASIC SHAPES --------------------------------------------------------

/****************************************************************************************************************/

//------------------------------------- SPRAY CAN BODY DETAILS -----------------------------------------------------------------

/************************* -- Spray Can Body Rim Edges TOP -- ***************************************/
/*** Set needed transformations before drawing the basic mesh.  ***/
/*** This same ordering of code should be used for transforming ***/
/*** and drawing all the basic 3D shapes.						***/

/******************************************************************/
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.6f, 0.1f, 1.6f);		// Scaling rim edges to be thin but slightly larger than the cylinder body

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 6.0f, -3.0f);		// positioning at a top rim position

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.65f, 0.65f, 0.68f, 1.0f);		// Setting the rim to be DARK GREY
	SetShaderTexture("14_stainless");					//setting a stainless texture
	SetShaderMaterial("steel");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/************************ End of Spray Can Body Rim TOP ****************************************/


/************************* -- Spray Can Body Rim Edges BOTTOM -- ***************************************/
/*** Set needed transformations before drawing the basic mesh.  ***/
/*** This same ordering of code should be used for transforming ***/
/*** and drawing all the basic 3D shapes.						***/

/******************************************************************/
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.6f, 0.1f, 1.6f);		// Scaling rim edges to be thin but slightly larger than the cylinder body

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 0.0f, -3.0f);		// positioning at a bottom rim position

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.65f, 0.65f, 0.68f, 1.0f);		// Setting the rim to be DARK GREY
	SetShaderTexture("14_stainless");					//setting a stainless texture
	SetShaderMaterial("steel");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/************************ End of Spray Can Body Rim ****************************************/


/************************* --Spray Can Body Green Label TOP-- ***************************************/
/*** Set needed transformations before drawing the basic mesh.  ***/
/*** This same ordering of code should be used for transforming ***/
/*** and drawing all the basic 3D shapes.						***/

/******************************************************************/
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 0.5f);		// Scaling Label so its smaller than the diameter than the cylinder

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 4.7f, -2.0f);		// positioning around top part of cylinder and outward to mimic a label

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader	
	//SetShaderColor(0.2f, 0.7f, 0.2f, 1.0f);			// Setting the label to GREEN
	SetShaderTexture("20_green_textured_paper");	//label is made out of paper so added a paper look alike texture

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/************************ End of Spray Can Body Green Label TOP ****************************************/


	/************************* --Spray Can Body Green Label BOTTOM-- ***************************************/
/*** Set needed transformations before drawing the basic mesh.  ***/
/*** This same ordering of code should be used for transforming ***/
/*** and drawing all the basic 3D shapes.						***/

/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 0.5f);		// Scaling Label so its smaller than the diameter than the cylinder

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.0f, 0.7f, -2.0f);		// positioning around bottom part of cylinder and outward to mimic a label

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.2f, 0.7f, 0.2f, 1.0f);		// Setting the label to GREEN
	SetShaderTexture("20_green_textured_paper");	//label is made out of paper so added a paper look alike texture

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/************************ End of Spray Can Body Green Label TOP ****************************************/

//------------------------------------- END OF SPRAY CAN BODY DETAILS -----------------------------------------------------------------


//***********************************************----- LAPTOP -----*************************************************************************

	/************************* LAPTOP BODY***************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 0.2f, 7.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.0f, 0.2f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the active color values in the shader (RGBA)
	SetShaderColor(0.18f, 0.18f, 0.20f, 1.0f);
	SetShaderTexture("21_laptop_plastic");


	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/************************* LAPTOP BODY ***************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 0.2f, 7.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 110.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.8f, 3.0f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the active color values in the shader (RGBA)
	SetShaderColor(0.18f, 0.18f, 0.20f, 1.0f);
	SetShaderTexture("21_laptop_plastic");

	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/**************************** LAPTOP SCREEN ************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 0.2f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 110.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.8f, 3.1f, 3.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the active color values in the shader (RGBA)
	//SetShaderColor(0.03f, 0.04f, 0.06f, 1.0f);
	SetShaderTexture("22_laptop_screen");

	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

//------------------------------------- END OF LAPTOP -----------------------------------------------------------------
//
//***********************************************----- PAPER TOWEL HOLDER -----*************************************************************************

	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 0.1f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 0.1f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the active color values in the shader (RGBA)
	//SetShaderColor(1, 1, 1, 1);
 	SetShaderTexture("22_light_wood");
	SetShaderMaterial("balsaWood");

	m_basicMeshes->DrawCylinderMesh();


	//***********************************************----- PAPER TOWEL HOLDER LONG ROD -----*************************************************************************

	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 10.0f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 0.0f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the active color values in the shader (RGBA)
	//SetShaderColor(0, 1, 1, 1);
	SetShaderTexture("22_light_wood");
	SetShaderMaterial("balsaWood");
 
	m_basicMeshes->DrawCylinderMesh();

	//***********************************************----- PAPER TOWEL HOLDER SHORT ROD -----*************************************************************************


	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 4.0f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.0f, 0.0f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the active color values in the shader (RGBA)
	//SetShaderColor(1, 1, 0, 1);
 	SetShaderTexture("22_light_wood");
	SetShaderMaterial("balsaWood");

	m_basicMeshes->DrawCylinderMesh();

	//***********************************************----- PAPER TOWEL HOLDER LONG ROD UPPER DETAIL-----*************************************************************************

	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.6f, 0.8f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 10.0f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the active color values in the shader (RGBA)
	//SetShaderColor(1, 1, 1, 1);
    SetShaderTexture("22_light_wood");
	SetShaderMaterial("balsaWood");

	m_basicMeshes->DrawSphereMesh();

//------------------------------------- END OF PAPER TOWEL HOLDER -----------------------------------------------------------------

//***********************************************----- RECORD SITTING ONTOP OF RECORD SLEEVE -----***********************************************************

//***********************************************----- RECORD SLEEVE -----*************************************************************************

	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 0.2f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.2f, 4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the active color values in the shader (RGBA)
	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("1_abstract");

	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

//***********************************************----- RECORD -----*************************************************************************

	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.8f, 0.1f, 2.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.4f, 4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the active color values in the shader (RGBA)
	SetShaderColor(0, 1, 1, 1);

	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

//***********************************************----- RECORD INNER AREA-----*************************************************************************

	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.1f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.5f, 4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the active color values in the shader (RGBA)
	SetShaderColor(0 ,0, 0, 1);

	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

//------------------------------------- RECORD SITTING ONTOP OF RECORD SLEEVE -----------------------------------------------------------------

}


