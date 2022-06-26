/*****************************************************************************

	HolderClass


    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __HolderClass_H__
#define __HolderClass_H__


namespace FlexYCF
{
	template<class T>
	class HolderClass
	{
	protected:
		HolderClass()
		{
		}

        /**
            @brief Copy construct from an existing instance.

            @param original The original instance to copy.
        */
        inline HolderClass(HolderClass const& original) : 
            m_holdee(original.m_holdee)
        {
        }

        template<class A>
		explicit HolderClass(const A& args):
			m_holdee(args)
		{
		}
		
		inline const T& holdee() const
		{	
			return m_holdee;
		}

	private:
		T	m_holdee;
	};

}

//	Helper macro to define holder classes.
#define DEFINE_HOLDER_CLASS( class_name__ )						\
	typedef HolderClass<class_name__>	class_name__##Holder;

#endif	//	 __HolderClass_H__