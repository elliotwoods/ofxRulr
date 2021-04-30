#include "pch_RulrNodes.h"
#include "BoardInWorld.h"
#include "AbstractBoard.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			BoardInWorld::BoardInWorld()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				BoardInWorld::getTypeName() const
			{
				return "Item::BoardInWorld";
			}

			//----------
			void 
				BoardInWorld::init()
			{
				RULR_NODE_UPDATE_LISTENER;

				this->addInput<AbstractBoard>();

				this->onDrawObject += [this]() {
					auto board = this->getInput<AbstractBoard>();
					if (board) {
						board->drawObject();
					}
				};
			}

			//----------
			void
				BoardInWorld::update()
			{
			}
		}
	}
}
