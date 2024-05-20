#pragma once

#include "reflect.hpp"

namespace Meow
{
    namespace reflect
    {
        /**
         * @brief Pointer to base class. It is initialized by derived class instance, and it also stores derived class name.
         * 
         * Pointing to base class is to support polymorphism. Storing derived class name is to 
         * 
         * @tparam T Base class name.
         */
        template<typename T>
        class ReflectionPtr
        {
            template<typename U>
            friend class ReflectionPtr;

        public:
            ReflectionPtr(std::string type_name, T* instance)
                : m_type_name(type_name)
                , m_instance(instance)
            {}
            ReflectionPtr()
                : m_type_name()
                , m_instance(nullptr)
            {}

            ReflectionPtr(const ReflectionPtr& dest)
                : m_type_name(dest.m_type_name)
                , m_instance(dest.m_instance)
            {}

            template<typename U /*, typename = typename std::enable_if<std::is_safely_castable<T*, U*>::value>::type */>
            ReflectionPtr<T>& operator=(const ReflectionPtr<U>& dest)
            {
                if (this == static_cast<void*>(&dest))
                {
                    return *this;
                }
                m_type_name = dest.m_type_name;
                m_instance  = static_cast<T*>(dest.m_instance);
                return *this;
            }

            template<typename U /*, typename = typename std::enable_if<std::is_safely_castable<T*, U*>::value>::type*/>
            ReflectionPtr<T>& operator=(ReflectionPtr<U>&& dest)
            {
                if (this == static_cast<void*>(&dest))
                {
                    return *this;
                }
                m_type_name = dest.m_type_name;
                m_instance  = static_cast<T*>(dest.m_instance);
                return *this;
            }

            ReflectionPtr<T>& operator=(const ReflectionPtr<T>& dest)
            {
                if (this == &dest)
                {
                    return *this;
                }
                m_type_name = dest.m_type_name;
                m_instance  = dest.m_instance;
                return *this;
            }

            ReflectionPtr<T>& operator=(ReflectionPtr<T>&& dest)
            {
                if (this == &dest)
                {
                    return *this;
                }
                m_type_name = dest.m_type_name;
                m_instance  = dest.m_instance;
                return *this;
            }

            std::string getTypeName() const { return m_type_name; }

            void setTypeName(std::string name) { m_type_name = name; }

            bool operator==(const T* ptr) const { return (m_instance == ptr); }

            bool operator!=(const T* ptr) const { return (m_instance != ptr); }

            bool operator==(const ReflectionPtr<T>& rhs_ptr) const { return (m_instance == rhs_ptr.m_instance); }

            bool operator!=(const ReflectionPtr<T>& rhs_ptr) const { return (m_instance != rhs_ptr.m_instance); }

            template<
                typename T1 /*, typename = typename std::enable_if<std::is_safely_castable<T*, T1*>::value>::type*/>
            explicit operator T1*()
            {
                return static_cast<T1*>(m_instance);
            }

            template<
                typename T1 /*, typename = typename std::enable_if<std::is_safely_castable<T*, T1*>::value>::type*/>
            operator ReflectionPtr<T1>()
            {
                return ReflectionPtr<T1>(m_type_name, (T1*)(m_instance));
            }

            template<
                typename T1 /*, typename = typename std::enable_if<std::is_safely_castable<T*, T1*>::value>::type*/>
            explicit operator const T1*() const
            {
                return static_cast<T1*>(m_instance);
            }

            template<
                typename T1 /*, typename = typename std::enable_if<std::is_safely_castable<T*, T1*>::value>::type*/>
            operator const ReflectionPtr<T1>() const
            {
                return ReflectionPtr<T1>(m_type_name, (T1*)(m_instance));
            }

            T* operator->() { return m_instance; }

            T* operator->() const { return m_instance; }

            T& operator*() { return *(m_instance); }

            T* getPtr() { return m_instance; }

            T* getPtr() const { return m_instance; }

            const T& operator*() const { return *(static_cast<const T*>(m_instance)); }

            T*& getPtrReference() { return m_instance; }

            operator bool() const { return (m_instance != nullptr); }

        private:
            std::string m_type_name {""};
            T*          m_instance {nullptr};
        };
    } // namespace reflect

} // namespace Meow
