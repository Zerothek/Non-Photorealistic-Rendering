#pragma once

#include <Component/SimpleScene.h>

#define WIN_API_FILE_BROWSING

class NonPhotorealisticRendering : public SimpleScene
{
	public:
		NonPhotorealisticRendering();
		~NonPhotorealisticRendering();

		void Init() override;

	private:
		void FrameStart() override;
		void Update(float deltaTimeSeconds) override;
		void FrameEnd() override;

		void OnInputUpdate(float deltaTime, int mods) override;
		void OnKeyPress(int key, int mods) override;
		void OnKeyRelease(int key, int mods) override;
		void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
		void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
		void OnWindowResize(int width, int height) override;
		
		void OpenDialogue();
		void OnFileSelected(std::string fileName);

		// Processing Effects
		int NonPhotorealisticRendering::Similar(int curr[3], int neighbour[3]);

		void Segmentation();
		void GrayScale();
		void SaveImage(std::string fileName);

	private:
		Texture2D *originalImage;
		Texture2D *processedImage;

		bool flip;
		int outputMode;
		bool gpuProcessing;
		bool saveScreenToImage;
};
