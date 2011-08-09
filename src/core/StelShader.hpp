#ifndef STELSHADER_HPP
#define STELSHADER_HPP

//#include "VecMath.hpp"

class StelShader {
public:
        StelShader();
        ~StelShader();

        int uniformLocation (const char *name) const;
        int attributeLocation (const char *name) const;

        void setUniform (int location, int value);
        void setUniform (int location, int x, int y);
        void setUniform (int location, int x, int y, int z);
        void setUniform (int location, int x, int y, int z, int w);

        void setUniform (int location, float value);
        void setUniform (int location, float x, float y);
        void setUniform (int location, float x, float y, float z);
        void setUniform (int location, float x, float y, float z, float w);

        bool load(const char* vertexFile, const char* pixelFile);
        bool use() const;

        friend bool useShader (const StelShader *shader);

private:
        unsigned int vertexShader;
        unsigned int pixelShader;
        unsigned int program;
};

bool useShader(const StelShader *shader);
#endif // STELSHADER_HPP
