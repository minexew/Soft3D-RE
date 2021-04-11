/* ================================================================
 * 版权所有 2006-2009 成都丁果科技有限公司，保留所有权利。
 * 
 * 允许对代码和二进制文件的使用、改动、再发布，但必须满足以下条件：
 * 
 * 1. 再发布的源代码，必须保留完整的版权信息。
 * 
 * 2. 再发布的二进制文件，必须产生完整的版权信息。
 * 
 * 3. 再发布版本中包括的最终用户文档，必须包括以下信息：
 * “本产品包括丁果科技所开发的软件内容。”
 * 或者，本信息可以出现在软件本身，例如第三方的声明信息中。
 *
 * 4. 使用本软件的产品，不得使用“Soft2D”或“S2D”作为产品名称，也不得在
 * 产品名称中包括“Soft2D”或“S2D”的字样，除非事先获得丁果科技的书面许可。
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL DINGOO GAMES OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 */ 
#include "../inc/Texturemanager.h"

//--------------------------------------------------------------------------------------------------------
//### interfaces:
//--------------------------------------------------------------------------------------------------------
TextureManager::TextureManager( void ) : m_nbTextures(0)
{
	memset( (Char*)m_TexturePool, 0, sizeof(m_TexturePool) );
}
//--------------------------------------------------------------------------------------------------------
TextureManager::~TextureManager( void )
{
	FreeAll();
}
//--------------------------------------------------------------------------------------------------------
/// find texture, will auto load if it's not exist
Texture*	TextureManager::FindOrLoadTexture( const Char* name, const Char* path )
{
	// look for exist
	TexSlot* tex = m_TexturePool;
	s32 i = 0;
	for( ; i<m_nbTextures; ++i, ++tex ) {
		if( s2d::strcmp( tex->name, name ) == 0 ) {
			return &tex->tex;
		}
	}

	if( i >= MAX_TEXTURES ) {//overflow
		return NULL;
	}

	// not exist, so load it

	// make full name first
	Char fname[256];
	s2d::strcpy( fname, name );
	s2d::strcat( fname, _LS(".s3dtex") );

	// get filename with path
	Char pathname[256];
	if( path == NULL ) {
		s2d::strcpy( pathname, fname );
	}
	else {
		s2d::strcpy( pathname, path );
		s2d::strcat( pathname, fname );
	}

	if( tex->tex.CreateFromFile( pathname ) != S2D_OK ) {//create texture failed
		return NULL;
	}
	s2d::strcpy( tex->name, name );
	m_nbTextures++;


	return &tex->tex;
}
//--------------------------------------------------------------------------------------------------------
/// free all textures
void		TextureManager::FreeAll( void )
{
	s32 i;
	// look for exist
	TexSlot* texs = m_TexturePool;
	for( i=0; i<m_nbTextures; ++i, ++texs ) {
		// free all
		texs->tex.Free();
	}
	m_nbTextures = 0;
}

