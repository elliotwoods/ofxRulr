#pragma once

namespace ofxDigitalEmulsion {
	namespace Utils {
		template<typename BaseType>
		class Set : public vector<shared_ptr<BaseType> > {
		public:
		public:
			template<typename T>
			shared_ptr<T> get()  {
				for(auto item : * this) {
					auto castItem = dynamic_pointer_cast<T>(item);
					if (castItem != NULL) {
						return castItem;
					}
				}
				ofLogError("ofxDigitalEmulsion") << "Item of type [" << T().getTypeName() << "] could not be found in Set<" << typeid(BaseType).name() << ">";
				return shared_ptr<T>();
			}

			void update() {
				for(auto item : * this) {
					item->update();
				}
			}

			void add(shared_ptr<BaseType> item) {
				this->push_back(item);
			}
		};
	}
}