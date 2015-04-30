#ifndef Sextets_Threads_SingleUseBox_h
#define Sextets_Threads_SingleUseBox_h

namespace Sextets
{
	namespace Threads
	{
		// An object that holds a pointer and returns it only to the first
		// caller of `Claim()`. All subsequent calls to `Claim()` will
		// return `NULL`. `Claim()` is interlocked for thread safety.
		//
		// If the responsibility for deleting the pointer is to be passed to
		// the winning `Claim()` caller, it is probably best to let
		// `deleteIfDestroyedBeforeClaimed` be true. This way, if there has
		// been no call to `Claim()` by the time this object is deleted,
		// this object will delete it as well.
		//
		// Conversely, if the pointer is not to be deleted, make sure to set
		// `deleteIfDestroyedBeforeClaimed` to `false`.
		//
		// If a `NULL` pointer is passed, then all calls to `Claim()` will
		// return `NULL` and `deleteIfDestroyedBeforeClaimed` is ignored.
		template<typename T> class SingleUseBox
		{
			private:
				template<typename U> class _Impl;
				template<typename U> friend class _Impl;
				SingleUseBox()
				{
				}

			public:
				virtual ~SingleUseBox()
				{
				}
				virtual T* Claim() = 0;

				static SingleUseBox<T> * Create(T* item, bool deleteIfDestroyedBeforeClaimed = true);
		};
	}
}

#endif
