#ifndef  Singleton_H_AFX_Included
#define  Singleton_H_AFX_Included

template <typename T>
class CSingleton {
public:
	static T* instance();
};

template <typename T>
T* CSingleton<T>:: instance()
{
    static T _instance;
	return &_instance;
}

#endif   /* Singleton_H_AFX_Included */
