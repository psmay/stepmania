
#ifndef Sextets_Threads_SingleReadPointerVariable_h
#define Sextets_Threads_SingleReadPointerVariable_h

#include "arch/Sextets/Threads/InterlockedPointerVariable.h"

namespace Sextets
{
	namespace Threads
	{
		template <typename T> class SingleReadPointerVariable
		{
			private:
				volatile bool _invalidated;
				InterlockedPointerVariable<T> _v;
				bool _deleteIfUnclaimed;
			public:
				SingleReadPointerVariable(T* value, bool deleteIfUnclaimed = true) :
					_invalidated(false),
					_v(value),
					_deleteIfUnclaimed(deleteIfUnclaimed)
				{
				}

				virtual ~SingleReadPointerVariable()
				{
					if(_deleteIfUnclaimed) {
						T* p = Claim();
						if(p != NULL) {
							delete p;
						}
					}
				}

				virtual T* Claim()
				{
					if(_invalidated) {
						return NULL;
					}
					else {
						_invalidated = true;
						return _v.GetAndSet(NULL);
					}
				}
		};
	}
}



#endif
