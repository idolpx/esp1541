// Pi1541 - A Commodore 1541 disk drive emulator
// Copyright(C) 2018 Stephen White
//
// This file is part of Pi1541.
// 
// Pi1541 is free software : you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Pi1541 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Pi1541. If not, see <http://www.gnu.org/licenses/>.

#ifndef FileBrowser_H
#define FileBrowser_H
#include <assert.h>
#include "ff.h"
#include <vector>
#include "types.h"
#include "DiskImage.h"
#include "DiskCaddy.h"
#include "ROMs.h"
#include "ScreenBase.h"
#include "InputMappings.h"

#define VIC2_COLOUR_INDEX_BLACK		0
#define VIC2_COLOUR_INDEX_WHITE		1
#define VIC2_COLOUR_INDEX_RED		2
#define VIC2_COLOUR_INDEX_CYAN		3
#define VIC2_COLOUR_INDEX_MAGENTA	4
#define VIC2_COLOUR_INDEX_GREEN		5
#define VIC2_COLOUR_INDEX_BLUE		6
#define VIC2_COLOUR_INDEX_YELLOW	7
#define VIC2_COLOUR_INDEX_ORANGE	8
#define VIC2_COLOUR_INDEX_BROWN		9
#define VIC2_COLOUR_INDEX_PINK		10
#define VIC2_COLOUR_INDEX_DGREY		11
#define VIC2_COLOUR_INDEX_GREY		12
#define VIC2_COLOUR_INDEX_LGREEN	13
#define VIC2_COLOUR_INDEX_LBLUE		14
#define VIC2_COLOUR_INDEX_LGREY		15

#define STATUS_BAR_POSITION_Y (40 * 16 + 10)

#define KEYBOARD_SEARCH_BUFFER_SIZE 512

class FileBrowser
{
public:

	class BrowsableList;

	class BrowsableListView
	{
	public:
		BrowsableListView(BrowsableList* list, InputMappings* inputMappings, ScreenBase* screen, uint32_t columns, uint32_t rows, uint32_t positionX, uint32_t positionY, bool lcdPgUpDown)
			: list(list)
			, inputMappings(inputMappings)
			, screen(screen)
			, columns(columns)
			, rows(rows)
			, positionX(positionX)
			, positionY(positionY)
			, lcdPgUpDown(lcdPgUpDown)
			, highlightScrollOffset(0)
			, highlightScrollStartCount(0)
			, highlightScrollEndCount(0)
			, scrollHighlightRate()
		{
		}

		void Refresh();
		void RefreshLine(uint32_t entryIndex, uint32_t x, uint32_t y, bool selected);
		void RefreshHighlightScroll();
		bool CheckBrowseNavigation(bool pageOnly);

		BrowsableList* list;
		uint32_t offset;
		InputMappings* inputMappings;
		ScreenBase* screen;
		uint32_t columns;
		uint32_t rows;
		uint32_t positionX;
		uint32_t positionY;
		bool lcdPgUpDown;
		uint32_t highlightScrollOffset;
		uint32_t highlightScrollStartCount;
		uint32_t highlightScrollEndCount;
		float scrollHighlightRate;
	};

	class BrowsableList
	{
	public:
		BrowsableList();

		void Clear()
		{
			uint32_t index;
			entries.clear();
			current = 0;
			currentIndex = 0;
			for (index = 0; index < views.size(); ++index)
			{
				views[index].offset = 0;
			}
		}

		void AddView(ScreenBase* screen, InputMappings* inputMappings, uint32_t columns, uint32_t rows, uint32_t positionX, uint32_t positionY, bool lcdPgUpDown)
		{
			this->inputMappings = inputMappings;
			BrowsableListView view(this, inputMappings, screen, columns, rows, positionX, positionY, lcdPgUpDown);
			views.push_back(view);
		}

		void ClearSelections();

		void SetCurrent()
		{
			if (entries.size() > 0)
			{
				Entry* currentEntry = &entries[currentIndex];
				if (currentEntry != current)
				{
					current = currentEntry;
					currentHighlightTime = scrollHighlightRate;
				}
			}
			else
			{
				current = 0;
			}
		}

		struct Entry
		{
			Entry() : caddyIndex(-1)
			{
			}
			FILINFO filImage;
			FILINFO filIcon;
			int caddyIndex;
		};

		Entry* FindEntry(const char* name);
		int FindNextAutoName(char* basename);

		void RefreshViews();
		void RefreshViewsHighlightScroll();
		bool CheckBrowseNavigation();

		InputMappings* inputMappings;
		std::vector<Entry> entries;
		Entry* current;
		uint32_t currentIndex;
		float currentHighlightTime;
		float scrollHighlightRate;

		uint32_t lastUpdateTime;
		char searchPrefix[KEYBOARD_SEARCH_BUFFER_SIZE];
		uint32_t searchPrefixIndex;
		uint32_t searchLastKeystrokeTime;
		std::vector<BrowsableListView> views;
	};

	FileBrowser(InputMappings* inputMappings, DiskCaddy* diskCaddy, ROMs* roms, uint8_t* deviceID, bool displayPNGIcons, ScreenBase* screenMain, ScreenBase* screenLCD, float scrollHighlightRate);

	void SelectAutoMountImage(const char* image);
	void DisplayRoot();
	void Update();

	void RefeshDisplay();
	void DisplayDiskInfo(DiskImage* diskImage, const char* filenameForIcon);

	void DisplayStatusBar();

	void FolderChanged();
	void PopFolder();

	bool SelectionsMade() { return selectionsMade; }
	const char* LastSelectionName() { return lastSelectionName; }
	void ClearSelections();

	void ShowDeviceAndROM();
	void ShowDeviceAndROM( const char* ROMName );
	

	void ClearScreen();

	static const long int LSTBuffer_size = 1024 * 8;
	static unsigned char LSTBuffer[];

	static uint32_t Colour(int index);

	static void RefreshDevicesEntries(std::vector<FileBrowser::BrowsableList::Entry>& entries, bool toLower);

	bool MakeLST(const char* filenameLST);
	bool SelectLST(const char* filenameLST);

	void SetScrollHighlightRate(float value) { scrollHighlightRate = value; }

	void DeviceSwitched();

private:
	void DisplayPNG(FILINFO& filIcon, int x, int y);
	void RefreshFolderEntries();

	void UpdateInputFolders();
	//void UpdateInputDiskCaddy();

	void UpdateCurrentHighlight();
	//void RefeshDisplayForBrowsableList(FileBrowser::BrowsableList* browsableList, int xOffset, bool showSelected = true);
	bool FillCaddyWithSelections();

	bool AddToCaddy(FileBrowser::BrowsableList::Entry* current);
	bool AddImageToCaddy(FileBrowser::BrowsableList::Entry* current);

	bool CheckForPNG(const char* filename, FILINFO& filIcon);
	void DisplayPNG();

	bool SelectROMOrDevice(uint32_t index);

	// returns the volume index if at the root of a volume else -1
	int IsAtRootOfDevice();

	InputMappings* inputMappings;

	enum State
	{
		State_Folders,
		State_DiskCaddy
	} state;

	BrowsableList folder;
	DiskCaddy* diskCaddy;
	bool selectionsMade;
	const char* lastSelectionName;
	ROMs* roms;
	uint8_t* deviceID;
	bool displayPNGIcons;
	bool buttonChangedROMDevice;

	BrowsableList caddySelections;
#if not defined(EXPERIMENTALZERO)
	ScreenBase* screenMain;
#endif
	ScreenBase* screenLCD;
	float scrollHighlightRate;

	bool displayingDevices;
};
#endif
