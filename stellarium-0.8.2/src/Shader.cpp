#include "glew.h"
#include <gl/glaux.h>
#include <string>
#include <map>
#include <fstream>

typedef const char *LPCSTR;

class TShader
{
protected:
  std::map <LPCSTR,LPCSTR> TextureNamesTable;
  std::map <LPCSTR,LPCSTR>::iterator TextureIt;
  std::map <LPCSTR,float> FloatNamesTable;
  std::map <LPCSTR,float>::iterator FloatIt;
  bool isReady;
  GLint Location;
  GLhandleARB vxShader, frShader, prObject; 
  UINT TextureArray[10];     
  std::string VertexShaderSource,FragmentShaderSource;
public:
  TShader::TShader():isReady(false),VertexShaderSource(""),FragmentShaderSource("")
  {

  };
  TShader::~TShader()
  {
    if (vxShader)
      glDeleteObjectARB(vxShader);
    if (frShader)
      glDeleteObjectARB(frShader);
    if (prObject)
      glDeleteObjectARB(prObject); 
  }

  //-----------------Работа с текстурами--------------------------
  void AddTexture(LPCSTR strFileName, LPCSTR TextureName)
  {
    TextureNamesTable[strFileName]=TextureName;
  }
protected:
  void BMP_Texture(LPCSTR strFileName, int ID)
  {
    if(!strFileName)
      return;

    AUX_RGBImageRec *pBitMap = auxDIBImageLoad(strFileName);

    if (pBitMap == NULL)
      exit(0);

    glEnable(GL_TEXTURE_2D);

    glGenTextures(1, &TextureArray[ID]);
    glBindTexture(GL_TEXTURE_2D, TextureArray[ID]);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, pBitMap->sizeX, pBitMap->sizeY, GL_RGB, GL_UNSIGNED_BYTE, pBitMap->data);
  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glDisable(GL_TEXTURE_2D);

    if (pBitMap)										
    {
      if (pBitMap->data)								
      {
        free(pBitMap->data);						
      }
      free(pBitMap);									
    }
  }
  void EnableTextures()
  {
    TextureIt=TextureNamesTable.begin();
    for(int i=0;TextureIt!=TextureNamesTable.end();TextureIt++,i++) 
    {
      BMP_Texture((*TextureIt).first, i);
    }
  }
  void SinchroniseUniformTextures()
  {
    TextureIt=TextureNamesTable.begin();
    for(int i=0;TextureIt!=TextureNamesTable.end();i++,TextureIt++)
    {
      Location=glGetUniformLocation(prObject, (*TextureIt).second);
      glActiveTexture(GL_TEXTURE0+i);
      glBindTexture(GL_TEXTURE_2D, TextureArray[i]);
      glUniform1i(Location, i);
    }
  }
  //----------------Работа с float объектами-------------------
public:
  void AddUniformFloat(LPCSTR strProgramFloatName, float Value)
  {
    FloatNamesTable[strProgramFloatName]=Value;
  }
protected:
  void SinchroniseUniformFloat()
  {
    FloatIt=FloatNamesTable.begin();
    for(;FloatIt!=FloatNamesTable.end();FloatIt++)
    {
      Location=glGetUniformLocationARB(prObject, (*FloatIt).first);
      glUniform1f(Location, (*FloatIt).second);	
    }
    Location=glGetUniformLocationARB(prObject, "LightPosition");
    float *v=new float[3];
    v[0]=0;
    v[1]=0;
    v[2]=0;
    glUniform3fvARB(Location, 1, v);	
  }
  //----------------Работа с шейдерами-------------------------
public:
  void LoadVertexShaderSourse(std::string SourceFileName)
  {
    if (!isReady)
    {
      std::ifstream infile;
      char buff[255];
      infile.open(SourceFileName.c_str());
      std::string temp="";
      while (!infile.eof())
      {
        infile.getline(buff,sizeof(buff));
        temp+=buff;
      };
      infile.close();
      VertexShaderSource=temp;
    };
  }
  void LoadFragmentShaderSourse(std::string SourceFileName)
  {
    if (!isReady)
    {
      std::ifstream infile;
      char buff[255];
      infile.open(SourceFileName.c_str());
      std::string temp="";
      while (!infile.eof())
      {
        infile.getline(buff,sizeof(buff));
        temp+=buff;
      };
      infile.close();
      FragmentShaderSource=temp;
    };	
  }
protected:
  void CreateShaderProgram()
  {
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
      MessageBeep(1);
    };
    GLenum glErr;
    GLint  vertCompiled, fragCompiled;    // Результат компиляции
    GLint  linked;	         			  // Результат линковки
    // Создадим объекты шейдеров
    vxShader = glCreateShader(GL_VERTEX_SHADER_ARB);
    frShader = glCreateShader(GL_FRAGMENT_SHADER_ARB); 
    // Загрузим текст шейдеров
    const char* shaderstring = VertexShaderSource.c_str();
    const char** pshaderstring = &shaderstring; 
    glShaderSourceARB(vxShader, 1,pshaderstring, NULL);
    shaderstring = FragmentShaderSource.c_str();
    pshaderstring = &shaderstring; 
    glShaderSourceARB(frShader, 1,pshaderstring, NULL);
    // Компилируем
    glCompileShader(vxShader);
    glGetShaderiv(vxShader, GL_COMPILE_STATUS, &vertCompiled);
    glErr = glGetError();
    glCompileShader(frShader); 
    glGetShaderiv(vxShader, GL_COMPILE_STATUS, &fragCompiled);
    glErr = glGetError();
    // Проверим результат компиляции
    if (!vertCompiled || !fragCompiled)
      return ;
    // Создадим программный объкт
    prObject = glCreateProgram(); 
    glErr = glGetError();
    // Подсоединим к программному объекту скомпилированные шейдеры
    glAttachShader(prObject, vxShader);
    glErr = glGetError();
    glAttachShader(prObject, frShader); 
    glErr = glGetError();
    // Соберём (линкуем)
    glLinkProgramARB(prObject); 
    glGetProgramivARB(prObject, GL_LINK_STATUS, &linked);
    glErr = glGetError();
    // проверим результат линковки
    if (!linked)
      return;
    // Разрешим испоьзование программного объкта
    glErr = glGetError();
  }
public:
  void Enable()
  {
    if (!isReady)
    {
      CreateShaderProgram();
      EnableTextures();
      isReady=true;
    };
    glUseProgram(prObject); 
    SinchroniseUniformFloat();
    SinchroniseUniformTextures();
  }
  void Disable()
  {
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
  }
};