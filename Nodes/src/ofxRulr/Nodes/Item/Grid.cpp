#include "pch_RulrNodes.h"
#include "Grid.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			Grid::Grid()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				Grid::getTypeName() const
			{
				return "Item::Grid";
			}

			//----------
			void
				Grid::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;

				this->manageParameters(this->parameters);
			}

			//----------
			void
				Grid::update()
			{
				if (this->parameters.countX.get() != this->cachedParameters.countX.get()
					|| this->parameters.countY.get() != this->cachedParameters.countY.get()
					|| this->parameters.spacing.get() != this->cachedParameters.spacing.get()
					|| this->parameters.crosses.get() != this->cachedParameters.crosses.get())
				{
					this->meshStale = true;
				}

				if (this->meshStale) {
					this->buildMesh();
				}
			}

			//----------
			void
				Grid::drawObject()
			{
				this->mesh.draw();
			}

			//----------
			void
				Grid::buildMesh()
			{
				this->mesh.clear();

				this->mesh.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);

				if (this->parameters.crosses) {
						auto crossSize = this->parameters.spacing / 10.0f;;
						
						for (int i = 0; i <= this->parameters.countX; i++) {
							for (int j = 0; j <= this->parameters.countY; j++) {
								auto x = i * this->parameters.spacing;
								auto y = j * this->parameters.spacing;

								this->mesh.addVertex({ x - crossSize, y, 0 });
								this->mesh.addVertex({ x + crossSize, y, 0 });

								this->mesh.addVertex({ x, y - crossSize, 0 });
								this->mesh.addVertex({ x, y + crossSize, 0 });
							}
						}
				}
				else {
					// Horizontal lines
					{
						auto width = this->parameters.countX * this->parameters.spacing;
						this->mesh.addVertex({ 0, 0, 0 });
						this->mesh.addVertex({ width, 0, 0 });
						for (int j = 0; j < this->parameters.countY; j++) {
							auto y = (j + 1) * this->parameters.spacing;
							this->mesh.addVertex({ 0, y, 0 });
							this->mesh.addVertex({ width, y, 0 });

						}
					}

					// Vertical lines
					{
						auto height = this->parameters.countY * this->parameters.spacing;
						this->mesh.addVertex({ 0, 0, 0 });
						this->mesh.addVertex({ 0, height, 0 });
						for (int i = 0; i < this->parameters.countX; i++) {
							auto x = (i + 1) * this->parameters.spacing;
							this->mesh.addVertex({ x, 0, 0 });
							this->mesh.addVertex({ x, height, 0 });
						}
					}
				}
				

				// Mark as cached
				this->meshStale = false;
				this->cachedParameters.countX = this->parameters.countX;
				this->cachedParameters.countY = this->parameters.countY;
				this->cachedParameters.spacing = this->parameters.spacing;
				this->cachedParameters.crosses = this->parameters.crosses;
			}
		}
	}
}