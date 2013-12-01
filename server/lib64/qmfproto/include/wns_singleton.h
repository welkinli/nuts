/*
 * comm.h
 *
 *  Created on: 2012-10-18
 *      Author: himzheng
 */

#ifndef SINGLETON_H_
#define SINGLETON_H_
namespace WNS
{
    class noncopyable
    {
        protected:
            noncopyable (void)
            {

            }

            ~noncopyable (void)
            {

            }

        private:
            noncopyable (const noncopyable&);
            const noncopyable& operator= (const noncopyable&);
    };

    template <typename T>
    class CSingleton: public noncopyable
    {
        public:
            static T* Instance(){return &m_instance;}
        private:
            static T m_instance;
    };

    template<typename T>
    T CSingleton<T>::m_instance;

}
#endif /* SINGLETON_H_ */
