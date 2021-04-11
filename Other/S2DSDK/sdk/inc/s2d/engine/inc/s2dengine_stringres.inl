
#ifndef __S2DENGINE_STRINGRES_INL__
#define __S2DENGINE_STRINGRES_INL__

namespace s2d
{
//--------------------------------------------------------------------------------------------------------
//### VisString:

//--------------------------------------------------------------------------------------------------------
/// find next blank index
inline		s32		VisString::GetNextBlankIndex( 
							s32 aStrInd					//[in] start index to find
						)
{
	u32* pstart = m_pBuffer+aStrInd;
	u32* pbuf = pstart;
	u32* pEnd = m_pBuffer+m_iLength;
	while( pbuf < pEnd ){
		if( *pbuf == STR_BLANK || *pbuf == STR_NEWLINE ){
			if( pstart == pbuf ){
				return 1;
			}
			return pbuf - pstart;
		}
		++pbuf;
	}
	return pEnd - pstart;
}
//--------------------------------------------------------------------------------------------------------
//get original string length
inline		u32		VisString::GetLength( void )
{
	return m_iLength;
}
//--------------------------------------------------------------------------------------------------------
/// get original string buffer
inline		u32*	VisString::GetBuffer( void )
{
	return m_pBuffer;
}
//--------------------------------------------------------------------------------------------------------
/// set string
inline void			VisString::SetString(
						u32* aStr,			//string code buffer
						u32 aLength		//string length
					)
{
	m_iLength = aLength;
	m_iSize = m_iLength<<1;
	m_pBuffer = aStr;
}
/// clear string data
inline		void	VisString::Clear( void )
{
	m_iLength = 0;
}
//--------------------------------------------------------------------------------------------------------
//### StringManager:
//--------------------------------------------------------------------------------------------------------
/// get VisString count
inline		u32		StringManager::GetCount()
{
	return m_iStrCnt;
}
//--------------------------------------------------------------------------------------------------------
/// get VisString
inline		VisString*		StringManager::GetString(
								u32 aStrInd				//string index
							)
{
	aStrInd--;
	if(aStrInd>=m_iStrCnt || m_pStrList==NULL )return NULL;
	else return m_pStrList[aStrInd];
}
//--------------------------------------------------------------------------------------------------------
/// create new VisString with input string
inline		VisString*		StringManager::NewString(
								Char* aInput				//input string ,only use a-z and 0-9
							)
{
	return new VisString(aInput);
}
//--------------------------------------------------------------------------------------------------------
/// create new VisString of given length
inline		VisString*		StringManager::NewString(
								u32 aLength					//string length
							)
{
	return  new VisString(aLength);
}
//--------------------------------------------------------------------------------------------------------
/// set render target
inline		void			StringManager::SetVRAMPtr(
												Pixel* aTarg,			//[in] display target
												s32	 iSrcWid,			//[in] screen width
												s32	 iSrcHig			//[in] screen width
											)
{
	m_pVRAM = aTarg;
	m_iScreenWidth = iSrcWid;
	m_iScreenHeight = iSrcHig;
}
//--------------------------------------------------------------------------------------------------------
}; // namespace s2d
#endif//__S2DENGINE_STRINGRES_INL__
