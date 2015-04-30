
#include "global.h"
#include "RageThreads.h"
#include "arch/Sextets/Threads/SingleUseBox.h"

namespace Sextets
{
	namespace Threads
	{
		template<typename T0> template<typename T> class SingleUseBox<T0>::_Impl: public SingleUseBox<T>
		{
			private:
				// Interlocks Claim() before item is marked as claimed
				RageMutex mutex;

				// If the item has definitely already been retrieved, this
				// flag lets us skip locking and just return NULL. This flag
				// starts as false and can only be set true within the
				// mutexed section. So, if this is already true, don't
				// bother locking the mutex.
				volatile bool claimed;

				// The payload
				T* item;

				// Whether we destroy the payload before item is claimed
				bool deleteIfDestroyedBeforeClaimed;

			public:
				_Impl(T* item, bool deleteIfDestroyedBeforeClaimed) :
					item(item), deleteIfDestroyedBeforeClaimed(deleteIfDestroyedBeforeClaimed), claimed(false)
				{
				}

				virtual ~_Impl()
				{
					T* p;

					if(deleteIfDestroyedBeforeClaimed) {
						p = Claim();
						if(p != NULL) {
							delete p;
						}
					}
				}

				virtual T* Claim()
				{
					T* result;
					if(claimed) {
						result = NULL;
					}
					else {
						mutex.Lock();
						claimed = true;
						result = item;
						item = NULL;
						mutex.Unlock();
					}
					return result;
				}
		};

		template<typename T> SingleUseBox<T> * SingleUseBox<T>::Create(T* item, bool deleteIfDestroyedBeforeClaimed)
		{
			return new _Impl<T>(item, deleteIfDestroyedBeforeClaimed);
		}
	}
}


