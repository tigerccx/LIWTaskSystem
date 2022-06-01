#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

#include <iostream>

namespace LIW {
	namespace Util {
		template<class T, uint64_t Size>
		class LIWThreadSafeQueueSized {
		public:
			typedef uint64_t size_type;
		private:
			typedef std::lock_guard<std::mutex> lock_guard;
			typedef std::unique_lock<std::mutex> uniq_lock;
			const std::chrono::milliseconds c_max_wait = std::chrono::milliseconds(1);
		public:
			LIWThreadSafeQueueSized() = default;
			LIWThreadSafeQueueSized(const LIWThreadSafeQueueSized&) = delete;
			LIWThreadSafeQueueSized& operator=(const LIWThreadSafeQueueSized&) = delete;

			/// <summary>
			/// Push a value into queue immediately. 
			/// </summary>
			/// <param name="val"> Value to enqueue. </param>
			/// <returns> Is operation successful. </returns>
			bool push_now(const T& val) {
				lock_guard lk(__m_mtx_data);
				if (__m_back.load(std::memory_order_relaxed) - __m_front.load(std::memory_order_relaxed) < Size) {
					__m_queue[__m_back.fetch_add(1, std::memory_order_release) % Size] = val;
					__m_cv_nonempty.notify_one();
					return true;
				}
				else {
					return false;
				}
			}
			bool push_now(T&& val) {
				lock_guard lk(__m_mtx_data);
				if (__m_back.load(std::memory_order_relaxed) - __m_front.load(std::memory_order_relaxed) < Size) {
					__m_queue[__m_back.fetch_add(1, std::memory_order_release) % Size] = val;
					__m_cv_nonempty.notify_one();
					return true;
				}
				else {
					return false;
				}
			}

			/// <summary>
			/// Push a value into queue when not full. 
			/// </summary>
			/// <param name="val"> Value to enqueue. </param>
			/// <returns> Is operation successful. Unsuccess means operation terminated. </returns>
			bool push(const T& val) {
				uniq_lock lk_full(__m_mtx_data);
				while (__m_back.load(std::memory_order_relaxed) - __m_front.load(std::memory_order_relaxed) >= Size && __m_running) {
					__m_cv_nonfull.wait_for(lk_full, c_max_wait);
				}
				if (__m_running) {
					__m_queue[__m_back.fetch_add(1, std::memory_order_release) % Size] = val;
					__m_cv_nonempty.notify_one();
					return true;
				}
				else {
					return false;
				}
			}
			bool push(T&& val) {
				uniq_lock lk(__m_mtx_data);
				while (__m_back.load(std::memory_order_relaxed) - __m_front.load(std::memory_order_relaxed) >= Size && __m_running) {
					__m_cv_nonfull.wait_for(lk, c_max_wait);
				}
				if (__m_running) {
					__m_queue[__m_back.fetch_add(1, std::memory_order_release) % Size] = val;
					__m_cv_nonempty.notify_one();
					return true;
				}
				else {
					return false;
				}
			}

			/// <summary>
			/// Pop from queue immediately. 
			/// </summary>
			/// <param name="valOut"> Value dequeued. </param>
			/// <returns> Is operation successful. </returns>
			bool pop_now(T& valOut) {
				lock_guard lk(__m_mtx_data);
				if (__m_back.load(std::memory_order_relaxed) > __m_front.load(std::memory_order_relaxed)) {
					valOut = std::move(__m_queue[__m_front.fetch_add(1, std::memory_order_release) % Size]);
					__m_cv_nonfull.notify_one();
					return true;
				}
				else {
					return false;
				}
			}

			/// <summary>
			/// Pop from queue when not empty. 
			/// </summary>
			/// <param name="valOut"> Value dequeued. </param>
			/// <returns> Is operation successful. Unsuccess means operation terminated. </returns>
			bool pop(T& valOut) {
				uniq_lock lk(__m_mtx_data);
				while (__m_back.load(std::memory_order_relaxed) <= __m_front.load(std::memory_order_relaxed) && __m_running) {
					__m_cv_nonempty.wait_for(lk, c_max_wait);
				}
				if (__m_running) {
					valOut = std::move(__m_queue[__m_front.fetch_add(1, std::memory_order_release) % Size]);
					__m_cv_nonfull.notify_one();
					return true;
				}
				else {
					return false;
				}
			}

			/// <summary>
			/// Get a copy of the front of the queue. 
			/// </summary>
			/// <param name="valOut"> Copy of the front element. </param>
			/// <returns> Is operation successful. Unsuccess means queue empty. </returns>
			bool front(T& valOut) {
				lock_guard lk(__m_mtx_data);
				if (__m_front.load(std::memory_order_relaxed) != __m_back.load(std::memory_order_relaxed)) {
					valOut = __m_queue[__m_front.load(std::memory_order_relaxed) % Size];
					return true;
				}
				else {
					return false;
				}
			}
			/// <summary>
			/// Get a copy of the end of the queue. 
			/// </summary>
			/// <param name="valOut"> Copy of the last element. </param>
			/// <returns> Is operation successful. Unsuccess means queue empty. </returns>
			bool back(T& valOut) {
				lock_guard lk(__m_mtx_data);
				if (__m_front.load(std::memory_order_relaxed) != __m_back.load(std::memory_order_relaxed)) {
					valOut = __m_queue[__m_back.load(std::memory_order_relaxed) - 1 % Size];
					return true;
				}
				else {
					return false;
				}
			}
			/// <summary>
			/// Get size of queue. 
			/// </summary>
			/// <returns> Size of queue. </returns>
			inline typename size_type size() const { return __m_back.load(std::memory_order_acquire) - __m_front.load(std::memory_order_acquire); }
			/// <summary>
			/// Get if queue is empty. 
			/// </summary>
			/// <returns> Is queue empty. </returns>
			inline bool empty() const { return __m_back.load(std::memory_order_acquire) == __m_front.load(std::memory_order_acquire); }

			/// <summary>
			/// Block the thread until queue is empty. 
			/// </summary>
			inline void block_till_empty() {
				while (!empty()) { std::this_thread::yield(); }
			}
			/// <summary>
			/// Notify all pop() calls to stop blocking and exit. 
			/// </summary>
			inline void notify_stop() {
				__m_running = false;
				__m_cv_nonempty.notify_all();
				__m_cv_nonfull.notify_all();
			}

		protected:
			T __m_queue[Size];
			std::atomic<size_type> __m_front;
			//size_type __m_front = 0;
			std::atomic<size_type> __m_back;
			//size_type __m_back = 0;
		private:
			mutable std::mutex __m_mtx_data;
			std::condition_variable __m_cv_nonempty;
			std::condition_variable __m_cv_nonfull;
			bool __m_running = true;
		};
	}
}