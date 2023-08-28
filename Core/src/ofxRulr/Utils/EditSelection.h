#pragma once

#include "ofxLiquidEvent.h"

namespace ofxRulr {
	namespace Utils {
		/// <summary>
		/// Class for handling the ">>" to drill down into this selection
		/// </summary>
		/// <typeparam name="T"></typeparam>
		template<typename T>
		struct EditSelection
		{
			T* selection = nullptr;
			ofxLiquidEvent<void> onSelectionChanged;

			bool isSelected(const T* const item) {
				return item == this->selection;
			}
			void select(T* const item) {
				if (item != this->selection) {
					this->selection = item;
					this->onSelectionChanged.notifyListeners();
				}
			}
			void deselect() {
				this->select(nullptr);
			}
		};
	}
}
