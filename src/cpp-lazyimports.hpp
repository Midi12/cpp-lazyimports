#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <map>
#include <algorithm>
#include <exception>
#include <cassert>
#include <regex>
#include <vector>
#include <functional>

#if defined(_WIN64)
	#include <windows.h>
#elif defined(__linux__) or defined(__APPLE__)
	#include <dlfcn.h>
#else
	#error platform not supported
#endif

namespace cpp_lazyimports {
		
	namespace exceptions {

#if defined(_WIN64)
	#define GETLASTERROR GetLastError()
#elif defined(__linux__) or defined(__APPLE__)
	#define GETLASTERROR errno
#else
	#error platform not supported
#endif

		class lazyimportexception : public std::exception {
		public:
			lazyimportexception(const std::string& message)
			: _message(message), _last_error(GETLASTERROR)
			{}

			virtual const char* what() const noexcept override {
				return _message.c_str();
			}
			
			std::uint32_t last_error() const
			{
				return _last_error;
			}
			
			virtual ~lazyimportexception() = default;
		private:
			std::string _message;
			std::uint32_t _last_error;
		};

		class lazymoduleexception : public std::exception {
		public:
			lazymoduleexception(const std::string& message)
			: _message(message), _last_error(GETLASTERROR)
			{}

			virtual const char* what() const noexcept override {
				return _message.c_str();
			}
			
			std::uint32_t last_error() const
			{
				return _last_error;
			}
			
			virtual ~lazymoduleexception() = default;
		private:
			std::string _message;
			std::uint32_t _last_error;
		};
	}

	namespace helpers {
		static bool is_import_str(const std::string& str) {
			return !str.empty() && std::all_of(str.begin(), str.end(), [](const auto& c) -> bool { return isalnum(c) || c == '!' || c == '_' || c == '.' || c == '/'; /*todo : better check*/ });
		}

		static std::pair<std::string, std::string> split_import_string(const std::string& str) {
			//assert(is_import_str(str));

			std::regex reg("!");
			std::sregex_token_iterator iter(str.begin(), str.end(), reg, -1);
			std::sregex_token_iterator end;

			std::vector<std::string> splitted(iter, end);

			assert(splitted.size() == 2);

			return std::make_pair(splitted.at(0), splitted.at(1));
		}
	}

#if defined(_WIN64)
	struct WindowsLoader {
		static std::uintptr_t load_module(const std::string& module_name) {
			return reinterpret_cast<std::uintptr_t>(LoadLibraryA(module_name.c_str()));
		}

		static std::uintptr_t get_symbol(std::uintptr_t module_handle, const std::string& symbol_name) {
			return reinterpret_cast<std::uintptr_t>(GetProcAddress(reinterpret_cast<HMODULE>(module_handle), symbol_name.c_str()));
		}
	};
#elif defined(__linux__) or defined(__APPLE__)
	struct UnixLoader {
		static std::uintptr_t load_module(const std::string& module_name) {
			return reinterpret_cast<std::uintptr_t>(dlopen(module_name.c_str(), RTLD_NOW));
		}

		static std::uintptr_t get_symbol(std::uintptr_t module_handle, const std::string& symbol_name) {
			return reinterpret_cast<std::uintptr_t>(dlsym(reinterpret_cast<void *>(module_handle), symbol_name.c_str()));
		}
	};
#else
	#error platform not supported
#endif

	class lazyimport {
	public:
		lazyimport() = default;

		lazyimport(const std::string& name, std::uintptr_t ptr)
		: _name(name), _ptr(ptr)
		{}

		lazyimport(const lazyimport&) = default;

		lazyimport& operator= (const lazyimport&) = default;

		lazyimport(lazyimport&&) noexcept = default;

		~lazyimport() = default;

		template <typename ReturnType, typename ...Args>
		ReturnType operator()(Args&&... args) const {
			return call<ReturnType>(std::forward<Args>(args)...);
		}

		template <typename ReturnType, typename ...Args>
		ReturnType call(Args&&... args) const {
			return reinterpret_cast<ReturnType(*)(Args ...)>(_ptr)(std::forward<Args>(args)...);
		}

		operator bool() const {
			return _ptr != 0;
		}

		std::string name() const {
			return _name;
		}

		std::uintptr_t ptr() const {
			return _ptr;
		}
	private:
		std::string _name;
		std::uintptr_t _ptr = 0;
	};

	template <typename LoaderTraits>
	class basic_lazyimportcollection {
	public:
		basic_lazyimportcollection() = default;

		basic_lazyimportcollection(const basic_lazyimportcollection&) = default;

		basic_lazyimportcollection& operator= (const basic_lazyimportcollection&) = default;

		basic_lazyimportcollection(basic_lazyimportcollection&&) noexcept = default;

		~basic_lazyimportcollection() = default;

		void find_or_load(const std::uintptr_t& handle, const std::string& function_name, lazyimport& elem) {
			auto it = std::find_if(_collection.begin(), _collection.end(), [&function_name](const lazyimport& import) -> bool {
				std::hash<std::string> hash_fn;
				return hash_fn(function_name) == hash_fn(import.name());
			});
			
			if (it != _collection.end()) {
				elem = *it;
			} else {
				std::uintptr_t ptr = LoaderTraits::get_symbol(handle, function_name.c_str());
				
				if (ptr == 0) {
					std::string err = "cannot load function " + function_name;

#if defined(__linux__) or defined(__APPLE__)
					err += " (" + std::string(dlerror()) + ")";
#endif

					throw exceptions::lazyimportexception(err);
				}

				elem = _collection.emplace_back(function_name, ptr);
			}
		}
	private:
		std::vector<lazyimport> _collection;
	};

	template <typename LoaderTraits>
	class basic_lazymodule {
	public:
		basic_lazymodule() = default;

		basic_lazymodule(const std::string& name, std::uintptr_t hmod)
		: _name(name), _handle(hmod)
		{}

		basic_lazymodule(const basic_lazymodule&) = default;

		basic_lazymodule<LoaderTraits>& operator= (const basic_lazymodule& mod) = default;

		basic_lazymodule(basic_lazymodule&&) noexcept = default;

		~basic_lazymodule() = default;

		std::string name() const {
			return _name;
		}

		std::uintptr_t handle() const {
			return _handle;
		}

		void add(const std::string& function_name, lazyimport& import) {
			_imports.find_or_load(_handle, function_name, import);
		}
	private:
		std::string _name;
		std::uintptr_t _handle = 0;
		basic_lazyimportcollection<LoaderTraits> _imports;
	};

	template <typename LoaderTraits>
	class basic_lazymodulecollection {
	private:
		basic_lazymodulecollection() = default;
		~basic_lazymodulecollection() = default;
		basic_lazymodulecollection(const basic_lazymodulecollection&) = delete;
		basic_lazymodulecollection& operator= (const basic_lazymodulecollection&) = delete;
		basic_lazymodulecollection(basic_lazymodulecollection&&) noexcept = default;

	public:
		static basic_lazymodulecollection& instance() {
			static basic_lazymodulecollection instance;
			return instance;
		}

		lazyimport register_import(const std::string& module_name, const std::string& function_name) {
			lazyimport import;
			
			try {
				basic_lazymodule<LoaderTraits> module;
				find_or_load(module_name, module);
				module.add(function_name, import);
			} catch (exceptions::lazymoduleexception& modex) {
				throw modex;
			} catch (exceptions::lazyimportexception& impex) {
				throw impex;
			}

			return import;
		}

		lazyimport register_import(const std::string& path) {
			auto import_data = helpers::split_import_string(path);
			return register_import(import_data.first, import_data.second);
		}

		void find_or_load(const std::string& name, basic_lazymodule<LoaderTraits>& elem) {
			auto it = std::find_if(_collection.begin(), _collection.end(), [&name](const basic_lazymodule<LoaderTraits>& module) -> bool {
				std::hash<std::string> hash_fn;
				return hash_fn(name) == hash_fn(module.name());
			});
			
			if (it != _collection.end()) {
				elem = *it;
			} else {
				std::uintptr_t hmod = LoaderTraits::load_module(name.c_str());
				
				if (hmod == 0) {
					std::string err = "cannot load module " + name;

#if defined(__linux__) or defined(__APPLE__)
					err += " (" + std::string(dlerror()) + ")";
#endif
					throw exceptions::lazymoduleexception(err);
				}

				elem = _collection.emplace_back(basic_lazymodule<LoaderTraits>(name, hmod));
			}
		}

	private:
		using modulecollection = std::vector<basic_lazymodule<LoaderTraits>>;
		modulecollection _collection;
	};

#if defined(_WIN64)
	using lazymodule = basic_lazymodule<WindowsLoader>;
	using lazymodulecollection = basic_lazymodulecollection<WindowsLoader>;
#elif defined(__linux__) or defined(__APPLE__)
	using lazymodule = basic_lazymodule<UnixLoader>;
	using lazymodulecollection = basic_lazymodulecollection<UnixLoader>;
#endif

	namespace literals {
		lazyimport operator"" _lazy(const char *str, std::size_t len) {
			return lazymodulecollection::instance().register_import(std::string(str, len));
		}
	}
}

#define LAZYCALL(ReturnType, path, ...) \
	::cpp_lazyimports::lazymodulecollection::instance().register_import(path).call<ReturnType>(__VA_ARGS__)

#define LAZYLOAD(path) \
	::cpp_lazyimports::lazymodulecollection::instance().register_import(path)