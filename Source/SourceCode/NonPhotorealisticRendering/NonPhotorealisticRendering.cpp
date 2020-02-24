#include "NonPhotorealisticRendering.h"

#include <vector>
#include <iostream>

#include <Core/Engine.h>

using namespace std;

// Order of function calling can be seen in "Source/Core/World.cpp::LoopUpdate()"
// https://github.com/UPB-Graphics/SPG-Framework/blob/master/Source/Core/World.cpp

NonPhotorealisticRendering::NonPhotorealisticRendering()
{
	outputMode = 0;
	gpuProcessing = false;
	saveScreenToImage = false;
	window->SetSize(600, 600);
}

NonPhotorealisticRendering::~NonPhotorealisticRendering()
{
}

FrameBuffer *processed;

void NonPhotorealisticRendering::Init()
{
	// Load default texture fore imagine processing 
	originalImage = TextureManager::LoadTexture(RESOURCE_PATH::TEXTURES + "Cube/posx.png", nullptr, "image", true, true);
	processedImage = TextureManager::LoadTexture(RESOURCE_PATH::TEXTURES + "Cube/posx.png", nullptr, "newImage", true, true);

	{
		Mesh* mesh = new Mesh("quad");
		mesh->LoadMesh(RESOURCE_PATH::MODELS + "Primitives", "quad.obj");
		mesh->UseMaterials(false);
		meshes[mesh->GetMeshID()] = mesh;
	}

	static std::string shaderPath = "Source/SourceCode/NonPhotorealisticRendering/Shaders/";

	// Create a shader program for particle system
	{
		Shader *shader = new Shader("ImageProcessing");
		shader->AddShader((shaderPath + "VertexShader.glsl").c_str(), GL_VERTEX_SHADER);
		shader->AddShader((shaderPath + "FragmentShader.glsl").c_str(), GL_FRAGMENT_SHADER);

		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
}

void NonPhotorealisticRendering::FrameStart()
{
}

void NonPhotorealisticRendering::Update(float deltaTimeSeconds)
{
	ClearScreen();

	auto shader = shaders["ImageProcessing"];
	shader->Use();

	if (saveScreenToImage)
	{
		window->SetSize(originalImage->GetWidth(), originalImage->GetHeight());
	}

	int flip_loc = shader->GetUniformLocation("flipVertical");
	glUniform1i(flip_loc, saveScreenToImage ? 0 : 1);

	int screenSize_loc = shader->GetUniformLocation("screenSize");
	glm::ivec2 resolution = window->GetResolution();
	glUniform2i(screenSize_loc, resolution.x, resolution.y);

	int outputMode_loc = shader->GetUniformLocation("outputMode");
	glUniform1i(outputMode_loc, outputMode);

	int gpuProcessing_loc = shader->GetUniformLocation("outputMode");
	glUniform1i(outputMode_loc, outputMode);

	int locTexture = shader->GetUniformLocation("textureImage");
	glUniform1i(locTexture, 0);
	auto textureImage = (gpuProcessing == true) ? originalImage : processedImage;
	textureImage->BindToTextureUnit(GL_TEXTURE0);

	RenderMesh(meshes["quad"], shader, glm::mat4(1));

	if (saveScreenToImage)
	{
		saveScreenToImage = false;
		GLenum format = GL_RGB;
		if (originalImage->GetNrChannels() == 4)
		{
			format = GL_RGBA;
		}
		glReadPixels(0, 0, originalImage->GetWidth(), originalImage->GetHeight(), format, GL_UNSIGNED_BYTE, processedImage->GetImageData());
		processedImage->UploadNewData(processedImage->GetImageData());
		SaveImage("shader_processing_" + std::to_string(outputMode));

		float aspectRatio = static_cast<float>(originalImage->GetWidth()) / originalImage->GetHeight();
		window->SetSize(static_cast<int>(600 * aspectRatio), 600);
	}
}

void NonPhotorealisticRendering::FrameEnd()
{
	DrawCoordinatSystem();
}

void NonPhotorealisticRendering::OnFileSelected(std::string fileName)
{
	if (fileName.size())
	{
		std::cout << fileName << endl;
		originalImage = TextureManager::LoadTexture(fileName.c_str(), nullptr, "image", true, true);
		processedImage = TextureManager::LoadTexture(fileName.c_str(), nullptr, "newImage", true, true);

		float aspectRatio = static_cast<float>(originalImage->GetWidth()) / originalImage->GetHeight();
		window->SetSize(static_cast<int>(600 * aspectRatio), 600);
	}
}

void NonPhotorealisticRendering::GrayScale()
{
	unsigned int channels = originalImage->GetNrChannels();
	unsigned char* data = originalImage->GetImageData();
	unsigned char* newData = processedImage->GetImageData();

	if (channels < 3)
		return;

	glm::ivec2 imageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());

	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			int offset = channels * (i * imageSize.x + j);

			// Reset save image data
			char value = static_cast<char>(data[offset + 0] * 0.2f + data[offset + 1] * 0.71f + data[offset + 2] * 0.07);
			memset(&newData[offset], value, 3);
		}
	}

	processedImage->UploadNewData(newData);
}

int NonPhotorealisticRendering::Similar(int curr[3], int neighbour[3]) {
	int threshold = 30;
	int similarity = abs(curr[0] - neighbour[0]) + abs(curr[1] - neighbour[1]) + abs(curr[2] - neighbour[2]);
	if (similarity <= threshold)
		return similarity;
	return -1;
}

void NonPhotorealisticRendering::Segmentation()
{
	unsigned int channels = originalImage->GetNrChannels();
	unsigned char* data = originalImage->GetImageData();
	unsigned char* newData = processedImage->GetImageData();

	if (channels < 3)
		return;

	glm::ivec2 imageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());
	static int x = imageSize.x;
	int y = imageSize.y;

	int ***zones = new int**[imageSize.y];
	for (int i = 0; i < imageSize.y; i++)
	{
		zones[i] = new int*[imageSize.x];
	}

	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			zones[i][j] = new int [3];
		}
	}

	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			for (int k = 0; k < 3; k++) {
				zones[i][j][k] = -1;
			}
		}
	}

	int zone = 1;
	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			int offset = channels * (i * imageSize.x + j);

			// Reset save image data
			/*char value = static_cast<char>(data[offset + 0] * 0.2f + data[offset + 1] * 0.71f + data[offset + 2] * 0.07);
			memset(&newData[offset], value, 3);*/
			int up_neighbour[3] = { -1, -1, -1 }, left_neighbour[3] = { -1, -1, -1 }, left_up_neighbour[3] = { -1, -1, -1 };
			if (i > 0) {
				up_neighbour[0] = zones[i - 1][j][0];
				up_neighbour[1] = zones[i - 1][j][1];
				up_neighbour[2] = zones[i - 1][j][2];
			}

			if (j > 0) {
				left_neighbour[0] = zones[i][j - 1][0];
				left_neighbour[1] = zones[i][j - 1][1];
				left_neighbour[2] = zones[i][j - 1][2];
			}

			if (i > 0 && j > 0) {
				left_up_neighbour[0] = zones[i-1][j - 1][0];
				left_up_neighbour[1] = zones[i-1][j - 1][1];
				left_up_neighbour[2] = zones[i-1][j - 1][2];
			}

			int current_pixel[3] = { data[offset], data[offset + 1], data[offset + 2] };

			int minimum[3];

			if (up_neighbour[0] != -1 && up_neighbour[1] != -1 && up_neighbour[2] != -1) {
				int up_offset = channels * ((i - 1) * imageSize.x + j);
				//int up_pixel[3] = { newData[up_offset], data[up_offset + 1], data[up_offset + 2] };
				if (Similar(current_pixel, up_neighbour) != -1) {
					zones[i][j] = zones[i - 1][j];
					memset(&newData[offset], newData[up_offset], 1);
					memset(&newData[offset + 1], newData[up_offset + 1], 1);
					memset(&newData[offset + 2], newData[up_offset + 2], 1);
				}
			}
			
			if (left_neighbour[0] != -1 && left_neighbour[1] != -1 && left_neighbour[2] != -1) {
				int left_offset = channels * (i * imageSize.x + j - 1);
				//int left_pixel[3] = { data[left_offset], data[left_offset + 1], data[left_offset + 2] };
				if (Similar(current_pixel, left_neighbour) != -1 && Similar(current_pixel, left_neighbour) < Similar(current_pixel, up_neighbour)) {
					zones[i][j] = zones[i][j - 1];
					memset(&newData[offset], newData[left_offset], 1);
					memset(&newData[offset + 1], newData[left_offset + 1], 1);
					memset(&newData[offset + 2], newData[left_offset + 2], 1);
				}
			}

			if (zones[i][j][0] == -1 && zones[i][j][1] == -1 && zones[i][j][2] == -1) {
				zones[i][j][0] = data[offset];
				zones[i][j][1] = data[offset + 1];
				zones[i][j][2] = data[offset + 2];
				zone++;
			}
		}
	}

	processedImage->UploadNewData(newData);
}

void NonPhotorealisticRendering::SaveImage(std::string fileName)
{
	cout << "Saving image! ";
	processedImage->SaveToFile((fileName + ".png").c_str());
	cout << "[Done]" << endl;
}

// Read the documentation of the following functions in: "Source/Core/Window/InputController.h" or
// https://github.com/UPB-Graphics/SPG-Framework/blob/master/Source/Core/Window/InputController.h

void NonPhotorealisticRendering::OnInputUpdate(float deltaTime, int mods)
{
	// treat continuous update based on input
};

void NonPhotorealisticRendering::OnKeyPress(int key, int mods)
{
	// add key press event
	if (key == GLFW_KEY_F || key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE)
	{
		OpenDialogue();
	}

	if (key == GLFW_KEY_E)
	{
		gpuProcessing = !gpuProcessing;
		if (gpuProcessing == false)
		{
			outputMode = 0;
		}
		cout << "Processing on GPU: " << (gpuProcessing ? "true" : "false") << endl;
	}

	if (key - GLFW_KEY_0 >= 0 && key < GLFW_KEY_5)
	{
		outputMode = key - GLFW_KEY_0;

		if (gpuProcessing == false)
		{
			outputMode = 0;
			cout << "Processing on GPU: " << (gpuProcessing ? "true" : "false") << endl;
			Segmentation();
			//GrayScale();
		}
	}

	if (key == GLFW_KEY_S && mods & GLFW_MOD_CONTROL)
	{
		if (!gpuProcessing)
		{
			SaveImage("processCPU_" + std::to_string(outputMode));
		}
		else
		{
			saveScreenToImage = true;
		}
	}
};

void NonPhotorealisticRendering::OnKeyRelease(int key, int mods)
{
	// add key release event
};

void NonPhotorealisticRendering::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
	// add mouse move event
};

void NonPhotorealisticRendering::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
	// add mouse button press event
};

void NonPhotorealisticRendering::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
	// add mouse button release event
}

void NonPhotorealisticRendering::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
	// treat mouse scroll event
}

void NonPhotorealisticRendering::OnWindowResize(int width, int height)
{
	// treat window resize event
}
