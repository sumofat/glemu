//NOTE(Ray):Please refer to the readme before inclusion into your project

#if !defined(OPENGLEMU_H)

namespace OpenGLEmu
{
    extern OpenGLEmuState ogs;
    
    void Init();
    //CPU BUFFER
    void CreateCPUBufferAtBinding(uint64_t bindkey,memory_index size);
    void AddCPUBindingToBuffer(uint64_t buffer_key,uint64_t key);
    CPUBuffer* GetCPUBufferAtBinding(uint64_t bindkey);
    YoyoVector GetCPUBufferList();
    //UNIFORM API
    UniformBindResult AddUniformDataAtBinding(uint64_t bindkey,void* uniform_data,memory_index size);
    BufferOffsetResult GetUniformAtBinding(uint64_t bindkey,uint32_t index);
    
    //TEST API
    void APInit();
    void Execute(void* pass_in_c_buffer = nullptr);
    void PreFrameSetup();
    
    SamplerState GetSamplerStateWithDescriptor(SamplerDescriptor desc);
    SamplerDescriptor GetSamplerDescriptor();
    SamplerDescriptor GetDefaultDescriptor();
    DepthStencilDescription GetDefaultDepthStencilDescriptor();
        
    SamplerState GetDefaultSampler();
    GLProgram AddProgramFromSource(const char* v_s,const char* vs_name,const char* f_s,const char* fs_name,VertexDescriptor vd);
    GLProgram AddProgramFromMainLibrary(const char* vs_name,const char* fs_name,VertexDescriptor vd);
    GLProgram GetDefaultProgram();
    void CreateBufferAtBinding(uint64_t bindkey);
    void AddBindingToBuffer(uint64_t buffer_key,uint64_t key);
    TripleGPUBuffer* GetBufferAtBinding(uint64_t bindkey);
    YoyoVector GetBufferList();
    YoyoVector GetProgramList();
    GLProgram GetProgram(GLProgramKey key);
    GLProgram* GetProgramPtr(GLProgramKey key);    
    void GLBlending(uint64_t gl_src,uint64_t gl_dst);
    void UseProgram(GLProgram gl_program);
    void EnableScissorTest();
    void DisableScissorTest();
    void ScissorTest(int x,int y,int width,int height);
    void ScissorTest(float4 rect);
    void Viewport(int x, int y, int width, int height);
    void Viewport(float4 vp);
    void BindFrameBufferStart(GLTexture texture);
    void BindFrameBufferEnd();
    void DrawArrays(uint32_t current_count,uint32_t unit_size);
    void DrawArrayPrimitives(uint32_t current_count,uint32_t unit_size);
    GLTexture TexImage2D(void* texels,float2 dim,PixelFormat format,SamplerDescriptor sd,TextureUsage usage);
    GLTexture TexImage2D(void* texels,float2 dim,PixelFormat format,TextureUsage usage);

    void AddFragTextureBinding(GLTexture texture,uint32_t index);
    void AddFragTextureBinding(GLTexture texture,uint32_t tex_index,uint32_t sampler_index);

    void AddBufferBinding(uint64_t bind_key,uint64_t index,uint64_t offset);

#define SetUniformsFragment(Type) (Type*)OpenGLEmu::SetUniformsFragment_(sizeof(Type))
    void* SetUniformsFragment_(memory_index size);
#define SetUniformsVertex(Type) (Type*)OpenGLEmu::SetUniformsVertex_(sizeof(Type))
    void* SetUniformsVertex_(memory_index size);
    UniformBindingTableEntry GetUniEntryForDrawCall(uint32_t index);
//NOTE(Ray):THis is actually worse than what I had before... need to do either binding model or pass in model
    SamplerState SamplerParameter(SamplerMinMagFilter min,SamplerMinMagFilter mag,SamplerAddressMode s,SamplerAddressMode t);

//note tested yet
    void EnableStencilTest();
    void DisableStencilTest();
    void StencilMask(uint32_t mask);
    void StencilMaskSep(uint32_t front_or_back,uint32_t mask);
    void StencilFunc(CompareFunc func,uint32_t ref,uint32_t mask);
    void StencilFuncSep(uint32_t front_or_back,CompareFunc func,uint32_t ref,uint32_t mask);
    void StencilOperation(StencilOp sten_fail,StencilOp dpfail,StencilOp dppass);
    void StencilOperationSep(uint32_t front_or_back,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass);
    void StencilFuncAndOp(CompareFunc func,uint32_t ref,uint32_t mask,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass);
    void StencilFuncAndOpSep(uint32_t front_or_back,CompareFunc func,uint32_t ref,uint32_t mask,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass);
    DepthStencilState GetOrCreateDepthStencilState(DepthStencilDescription desc);
    void ClearBuffer(uint32_t buffer_bits);
    void ClearStencil(uint32_t value);
    void ClearColor(float4 value);
    void ClearColorAndStencil(float4 color,uint32_t stencil);

    DepthStencilDescription GetCurrentDepthStencilState();
    uint32_t GetCurrentStencilReferenceValue();
    
    //Stats
    uint32_t GetDepthStencilStateCount();

    //Debug
    void AddDebugSignPost(char* str);

    //Resource Management 
    void GLDeleteTexture(GLTexture* texture);
    bool GLIsValidTexture(GLTexture texture);
};

//NOTE(Ray):Probably a better place fo rthis is in the openglemu file
namespace RenderProgramCache
{
    void Init(uint32_t max_hash_states);
    bool DoesProgramExist(RenderShader* render_shader);
    //Returns key for later retrival
    uint64_t AddProgram(RenderShader* shaders,GLProgram* glp);
    GLProgram GetProgram(RenderShader* shaders);
    GLProgram GetSamplerStateByKey(uint64_t hash_key);
};
#include "glemu.cpp"
#define OPENGLEMU_H
#endif

