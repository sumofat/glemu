//GLEMU Project header
//The aim of this project is to provide an easy to use api for using legacy projects on newer apis.
//It is built on top of metalizer which is the multi platform part GLemu just provides a more abstracted "better" than opengl
//renderering api. 
//The main mission is to convert easily projects off of legacy graphics apis while still maintaining control for performance reasons.
//There is a possiblity however that the usage could expand beyond that if it seems reasonable to do so.
//For now we carry on with our mission..... starting 1000 yards into the storm unabashed we carry on to the wolves eyes. And even though
//it looks hungry, we are hungrier....

//TODO(Ray):NOTE(Ray):There is still a lot of code that is relevant to GLEMU in Spritebatch.h of metalizer project.

//Welcome to GLEMU

//Objectives:
//Short term: Create an opengl like rendering api that can convert legacy opengl to metal output mainly.
//1. perf not primary for short term as we mainly targeting 2d games at first.
//2. correct output will be the main focus
//3. adherance to the opengl standard is not primary. Improve what can be if possible make a more reasonable gl.
//in other words we dont need to stick to the opengl spec just take the good parts
//where it makes sense for converting projects.

//Long  term: Create a deffered rendering api that is able to create relatively efficient multithreaded rendering calls and validation
//based on a post built frame graph.
//1. Perf will be cricial in this stage and strategies to advance in that area will be extreme.
//2. At this point we will be adding completely new concepts and try to mix and match where it makes sense.

//Everything here is still WIP... for release info there will be a release notes doc.

//If you aquired this project as a maintainer every major release should be named.
//Look here for release names to use 
//First release name is 

//OpenGLEMU.h is a single file header file but requires project Metallizer project to run and render.
//Use: add include header to one of the or the first file in your project to use GLEmu.
//Above the include statement define GLEMU_IMPL

//!!!!!!!!!!!!!!!!!!!!!!!!!!READ FIRST!!!!!!!!!!!!!!!!!!!!!!!!!
//Notes and caveats and preamble:
//Short version: This lib may or may not (more likely not)make your opengl code go faster and you need to know what your
//doing to get fast results.

//Long version:
//Things to keep in mind when using this api.
//If you try to use this as you would regular opengl there will be issues you be well off to learn about the newer api's
//and understand what causes pipeline states to be generated and you can avoid many pitfalls.
//The main advantage is you avoid a lot of the boiler plate that comes with the newer api's but at a cost.
//You can avoid many of the pitfalls by constructiong your GLCalls by batching as much as possible your draw calls.
//But thats up to you based on your usage needs. Just something to keep in mind that if you do your rendering in a highly
//ineffecient order there is alil that we can do to mitagate that.  It all comes down to keep things that cause state tranisitions
//to a minimum and ordering your draw calls in a sane manner.   There are plans to do something like this in the backend but
//is still a micro optimization and will not save you if you order things in an ineffecient manner.

//There is still a lot missing and highly ineffecient.
//What is included so far.
//1. Very Basic rendering and binding.
//Nothing else!
//No compute no tesselation no nothing at the moment.

//So why shouldnt I use moltenGL?
//Well maybe you should.
//The advantages here are we are not neccessarily tied to a backend(someday).
//Not tied to a spec.
//"Simpler to use"?
//Can provide a callback to write your own specific "gldriver" or modify the original one.
//Able to experiment with our own possible better api that might operate at a slightly better level of abstraction.
//For quickly iterating deffering calls , analysing the frame and ordering calls in a way that might be more effecient.
//If all that doesnt interest you than you should use moltenGL

#if !defined(OPENGLEMU_H)
    
struct MatrixSetupParams
{
    //SpriteBatch* sb;
    void* command_buffer;
    TripleGPUBuffer* vertexbuffer;
};

struct MatrixPassInParams
{
    RenderCommandEncoder re;
//    SpriteBatch sb;
    Drawable current_drawable;
    float4 viewport;
    bool is_s_rect;
    ScissorRect s_rect;
    SamplerState sampler_state;
    RenderPipelineState pipeline_state;
    TripleGPUBuffer* vertexbuffer;
};

//glemu constant defines
#define GLEMU_MAX_ATLAS_PER_SPRITE_BATCH 10
#define GLEMU_MAX_PSO_STATES 4000
#define GLEMU_DEFAULT_TEXTURE_DELETE_COUNT 5

struct GLProgramKey
{
//    RenderShader s;
    uint64_t v;
    uint64_t f;
//    uint64_t ;
//    VertexDescriptor vd;
};

//#define METALIZER_INSERT_DEBUGSIGNPOST 0
struct UniformBindingTableEntry
{
    uint32_t call_index;
    uint32_t v_size;
    uint32_t f_size;
    void* v_data;
    void* f_data;
};

struct TextureBindingTableEntry
{
    uint32_t vertex_or_fragment;
    uint32_t call_index;
    uint32_t size;
    void* texture_ptr;
};

enum BufferBindTarget
{
    ArrayBuffer = 0,
    IndexBuffer = 1
};

struct BufferBindingTableEntry
{
    uint32_t index;//The index to bind to on the shader
    uint32_t offset;
    GPUBuffer buffer;//the binding id for the buffer in glemu
};

struct FragmentShaderTextureBindingTableEntry
{
    uint32_t sampler_index;
    uint32_t tex_index;
    GLTexture texture;
};

enum GLEMUBufferState
{
    glemu_bufferstate_none,//defualt0k
    glemu_bufferstate_start,//start means we are drawing NOW!1
    glemu_bufferstate_viewport_change,//means we are changin viewport size/location2
    glemu_bufferstate_blend_change,//3
    glemu_bufferstate_scissor_rect_change,//4
    glemu_bufferstate_scissor_test_enable,//5
    glemu_bufferstate_scissor_test_disable,//if disable default is full viewport settings.//6
    glemu_bufferstate_shader_program_change,//7
    glemu_bufferstate_bindbuffer,//8
    glemu_bufferstate_set_uniforms,//this is a lil different than opengl but lets you pass some data easily without creating an intermediate buffer9
    //limited to 4kb<  most uniforms are under that amount
//    glemu_bufferstate_binduniform_buffer,//Note implmenented because have no need for it yet.
//Depth and stencils
    glemu_bufferstate_stencil_enable,//10
    glemu_bufferstate_stencil_disable,//11
    glemu_bufferstate_stencil_mask,//12
    glemu_bufferstate_stencil_mask_sep,//13
    glemu_bufferstate_stencil_func,//14
    glemu_bufferstate_stencil_func_sep,//15
    glemu_bufferstate_stencil_op,//16
    glemu_bufferstate_stencil_op_sep,//17
    glemu_bufferstate_stencil_func_and_op,//18
    glemu_bufferstate_stencil_func_and_op_sep,//19
    glemu_bufferstate_clear_start,//20
    glemu_bufferstate_clear_end,//21

    glemu_bufferstate_clear_stencil_value,//22
    glemu_bufferstate_clear_color_value,//23
    glemu_bufferstate_clear_color_and_stencil_value,//24
    //Debug stuff
    glemu_bufferstate_debug_signpost,//25
    glemu_bufferstate_draw_arrays,//26
    glemu_bufferstate_end//27
};

struct GLEMUAtlasResource
{
    float2 dim;
    void* texels;
};

struct GLEMUAtlasEntry
{
    float2 dim;
    float2 uv[4];
};

struct GLEMUAtlasHeader
{
    char* name;
    float2 dim;
};

struct GLEMUAtlas
{
    GLEMUAtlasHeader header;
    GLEMUAtlasEntry entries[600];
    LoadedTexture texture;
    float2 uv[4];
};

//TODO(Ray):Actual Sprite into the texture
//rename and rehting a bit
struct GLEMUAtlasTexture
{
    GLEMUAtlas* atlas;//Re think using pointers here.
    GLEMUAtlasEntry* entry;
    Yostr file_name;
};

struct GLEMURenderWithMatrixCommand
{
    RenderMaterial material;
    float4x4 model_matrix;
    GPUBuffer buffer;
    GPUBuffer matrix_buffer;
    GPUBuffer atlas_index_buffer;
    GPUBuffer uniforms;
};

struct GLEMURenderCommandParams
{
    GPUBuffer* uniforms;
    SamplerState* sampler_state;
    ScissorRect s_rect;
    bool is_s_rect;

    //uniforms
    uint32_t buffer_index;
    uint32_t buffer_unit_size;
    void* buffer_ptr;
    uint32_t current_count;
	RenderMaterial material;
	PrimitiveTopologyClass topology;
};

/*
struct VertexShaderTextureBindingTableEntry
{
    uint32_t index;//The index to bind to on the shader
    uint32_t offset;
    GPUBuffer buffer;//the binding id for the buffer in glemu
    BufferBindTarget bind_target;
};

struct IndexBufferBindingTableEntry
{
    uint32_t index;//The index to bind to on the shader
    uint32_t offset;
    GPUBuffer buffer;//the binding id for the buffer in glemu
    BufferBindTarget bind_target;
};
*/

//TODO(Ray):Could do multiple of these for multithreading and join after done ...
//perhaps something for another day. For now single thread only.
struct DrawCallTables
{
    YoyoVector uniform_binding_table;
    YoyoVector texture_binding_table;
    YoyoVector buffer_binding_table;
};

struct GLTextureKey
{
    //void* api_internal_ptr;
    PixelFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t sample_count;
    StorageMode storage_mode;
    bool allowGPUOptimizedContents;
    uint64_t gl_tex_id;
};

//NOTE(Ray):The number we delete on this will be a deffered deletion scheme such that.
//anytime release is called on a texture it is added to the table and everytime a frame passes the count increases.
//So deletion of textures ON THE GPU are deffered until a safe time. Typically at least one frame later but could be more.
//Also the client can request to check and know without a doubt if the texture has been deleted for sure and if its scheduled
//to be deleted by simply checking the count.
struct ReleasedTextureEntry
{
    GLTextureKey tex_key;//Create a tex id from some backing store.
//    uint64_t texture_key;
    uint32_t delete_count;
    uint32_t current_count;
    memory_index thread_id;
    bool is_free;//TODO(Ray):Implement the meta data free list iterator scheme in AnythingCacheCode to get rid of this bool
};

struct UsedButReleasedEntry
{
    uint64_t texture_key;
    memory_index thread_id;    
};

struct ResourceManagementTables
{
//    YoyoVector released_textures_table;
    YoyoVector used_but_released_table;
    AnythingCache released_textures_table;
};

//Command types
struct GLEMURenderCommandList
{
    MemoryArena buffer;
    uint32_t count;
};

struct GLEMUCommandHeader
{
	GLEMUBufferState type;
};

struct GLEMUBlendCommand
{
	BlendFactor sourceRGBBlendFactor;
	BlendFactor destinationRGBBlendFactor;
};

struct GLEMUUseProgramCommand
{
	GLProgram program;
};

struct GLEMUScissorTestCommand
{
	bool is_enable;
};

struct GLEMUScissorRectCommand
{
	ScissorRect s_rect;
};

struct GLEMUViewportChangeCommand
{
	float4 viewport;
};

struct GLEMUFramebufferStart
{
	Texture texture;
};

struct GLEMUFramebufferEnd
{
	//nothing fo now
};

struct GLEMUStencilStateCommand
{
	bool is_enable;
};

struct GLEMUStencilMaskCommand
{
	uint32_t write_mask_value;
};

struct GLEMUStencilMaskSepCommand
{
	uint32_t front_or_back;
	uint32_t write_mask_value;
};

struct GLEMUStencilFunCommand
{
	CompareFunc compareFunction;
	uint32_t mask_value;
	uint32_t write_mask_value;
};

struct GLEMUStencilFunSepCommand
{
	uint32_t front_or_back;
	CompareFunc compareFunction;
	uint32_t mask_value;
	uint32_t write_mask_value;
};

struct GLEMUStencilOpCommand
{
	StencilOp stencil_fail_op;
	StencilOp depth_fail_op;
	StencilOp depth_stencil_pass_op;
};

struct GLEMUStencilOpSepCommand
{
	uint32_t front_or_back;
	StencilOp stencil_fail_op;
	StencilOp depth_fail_op;
	StencilOp depth_stencil_pass_op;
};

struct GLEMUStencilFuncAndOpCommand
{
	CompareFunc compareFunction;
	uint32_t write_mask_value;
	StencilOp stencil_fail_op;
	StencilOp depth_fail_op;
	StencilOp depth_stencil_pass_op;
	uint32_t mask_value;
};

struct GLEMUStencilFuncAndOpSepCommand
{
	uint32_t front_or_back;
	CompareFunc compareFunction;
	uint32_t write_mask_value;
	StencilOp stencil_fail_op;
	StencilOp depth_fail_op;
	StencilOp depth_stencil_pass_op;
	uint32_t mask_value;
};

struct GLEMUClearBufferCommand
{
	bool is_start;
	uint32_t write_mask_value;
};

struct GLEMUClearStencilCommand
{
	uint32_t write_mask_value;
};
struct GLEMUClearColorCommand
{
	float4 clear_color;
};

struct GLEMUClearColorAndStencilCommand
{
	float4 clear_color;
	uint32_t write_mask_value;
};

struct GLEMUAddDebugSignPostCommand
{
	char* string;
};

struct GLEMUDrawArraysCommand
{
	bool is_from_to;
	bool is_primitive_triangles;
	uint32_t uniform_table_index;
	float2 buffer_range;
	float2 texture_buffer_range;
	uint32_t current_count;
    PrimitiveTopologyClass topology;
};

namespace OpenGLEmu
{
//Variables that have been exposed or should possiblly exposed are here.
    extern uint32_t current_buffer_index;
	extern uint32_t current_count;
    extern uint32_t buffer_count;
    
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
    void Execute();
    void PreFrameSetup();
    
    SamplerState GetSamplerStateWithDescriptor(SamplerDescriptor desc);
    SamplerDescriptor GetSamplerDescriptor();
    SamplerDescriptor GetDefaultDescriptor();
    DepthStencilDescription GetDefaultDepthStencilDescriptor();
        
    SamplerState GetDefaultSampler();
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

    void AddBufferBinding(GPUBuffer buffer,uint64_t index,uint64_t offset);

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

