#ifndef GRAPHICS_HEADER
#define GRAPHICS_HEADER

#include <string>

class Graphics {
	private:
		Graphics();
		static Graphics* instance;
	public:
		static Graphics* Inst();
		void DrawDebug(std::string);

};

#endif
