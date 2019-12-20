#if !defined(GLEMU_STATE)

#define GLEMU_DEBUG 0
#define METALIZER_INSERT_DEBUGSIGNPOST 0
#define METALIZER_DEBUG_OUTPUT 1
//glemu constant defines
#define GLEMU_MAX_ATLAS_PER_SPRITE_BATCH 10
#define GLEMU_MAX_PSO_STATES 4000
#define GLEMU_DEFAULT_TEXTURE_DELETE_COUNT 50

#define CLEAR_COLOR_BIT (1 << 1)
#define CLEAR_STENCIL_BIT (1 << 2)
#define CLEAR_DEPTH_BIT (1 << 3)

struct GLFrameBufferInternal
{
    u32 id;
    YoyoBufferFixed color;
    RenderPassDepthAttachmentDescriptor depth;
    RenderPassStencilAttachmentDescriptor stencil;
};

//TODO(Ray):The naming of this is a leftover needs more thought 
struct MatrixPassInParams
{
    RenderCommandEncoder re;
    Drawable current_drawable;
    float4 viewport;
    bool is_s_rect;
    ScissorRect s_rect;
    SamplerState sampler_state;
    RenderPipelineState pipeline_state;
};

struct GLProgramKey
{
    uint64_t v;
    uint64_t f;
};

struct UniformBindingTableEntry
{
    uint32_t call_index;
    uint32_t v_size;
    uint32_t f_size;
    void* v_data;
    void* f_data;
    u32 v_index;
    u32 f_index;
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
    uint32_t g_i;//part of the gpu key for retrieval.
    uint32_t offset;
    
    //GPUBuffer buffer;//the binding id for the buffer in glemu
    uint64_t key;
};

struct FragmentShaderTextureBindingTableEntry
{
    uint32_t sampler_index;
    uint32_t tex_index;
    GLTexture texture;
};

enum GLEMUBufferState
{
    glemu_bufferstate_none,
    glemu_bufferstate_start,//start means we are drawing NOW!1
    glemu_bufferstate_viewport_change,//means we are changin viewport size/location2
    glemu_bufferstate_blend_change,
    glemu_bufferstate_blend_change_i,
    glemu_bufferstate_scissor_rect_change,
    glemu_bufferstate_scissor_test_enable,
    glemu_bufferstate_scissor_test_disable,//if disable default is full viewport settings.
    glemu_bufferstate_shader_program_change,
    glemu_bufferstate_bindbuffer,
    glemu_bufferstate_set_uniforms,//this is a lil different than opengl but lets you pass some data easily without creating an intermediate buffer9
    //limited to 4kb<  most uniforms are under that amount
//    glemu_bufferstate_binduniform_buffer,//Note implmenented because have no need for it yet.
    
//Depth and stencils
    glemu_bufferstate_depth_enable,
    glemu_bufferstate_depth_disable,
    glemu_bufferstate_depth_func,
    
    glemu_bufferstate_stencil_enable,
    glemu_bufferstate_stencil_disable,
    glemu_bufferstate_stencil_mask,
    glemu_bufferstate_stencil_mask_sep,
    glemu_bufferstate_stencil_func,
    glemu_bufferstate_stencil_func_sep,
    glemu_bufferstate_stencil_op,
    glemu_bufferstate_stencil_op_sep,
    glemu_bufferstate_stencil_func_and_op,
    glemu_bufferstate_stencil_func_and_op_sep,
    glemu_bufferstate_clear_start,
    glemu_bufferstate_clear_end,
    glemu_bufferstate_clear_color_target,

    glemu_bufferstate_clear_stencil_value,
    glemu_bufferstate_clear_color_value,
    glemu_bufferstate_clear_depth_value,
    glemu_bufferstate_clear_color_and_stencil_value,
    //Debug stuff
    glemu_bufferstate_debug_signpost,
    glemu_bufferstate_draw_arrays,
    glemu_bufferstate_draw_elements,
    glemu_bufferstate_framebuffer_bind,
    
    glemu_bufferstate_end
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
    uint32_t pad;
};

struct GLEMUBlendCommand
{
	BlendFactor sourceRGBBlendFactor;
	BlendFactor destinationRGBBlendFactor;
};

struct GLEMUBlendICommand
{
	BlendFactor sourceRGBBlendFactor;
	BlendFactor destinationRGBBlendFactor;
    u32 index;
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

struct GLEMUBindFrameBufferCommand
{
	u64 id;
};

struct GLEMUFramebufferStart
{
	Texture texture;
};

struct GLEMUFramebufferEnd
{
    uint32_t placeholder;
	//nothing fo now
};

//depth
struct GLEMUDepthStateCommand
{
	bool is_enable;
};

struct GLEMUDepthFuncCommand
{
	CompareFunc func;
};

//stencil
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

struct GLEMUClearColorBufferCommand
{
    u32 index;
};

struct GLEMUClearStencilCommand
{
	uint32_t write_mask_value;
};

struct GLEMUClearDepthCommand
{
	uint32_t value;
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
    u64 offset;
    //for elements
    u64 element_buffer_id;
    u32 index_type;
};

struct OpenGLEmuState
{
    GLEMURenderCommandList command_list;
    //Samplers
    SamplerDescriptor defaults;
    SamplerDescriptor samplerdescriptor;
    SamplerState default_sampler_state;
    
    DepthStencilDescription default_depth_stencil_description;
    DepthStencilDescription current_depth_stencil_description;
    
    DepthStencilDescription ds;
    uint32_t current_reference_value;

    bool is_stencil_enabled;
    bool is_depth_enabled;
    DepthStencilState default_depth_stencil_state;
    DepthStencilState curent_depth_stencil_state;
    
    RenderShader default_shader;
    GLProgram default_program;
    GLProgram current_program;
    
    AnythingCache buffercache;
    AnythingCache gpu_buffercache;
    //NOTE(Ray):intention was variable sized buffers for uniforms but decided on not using
    //Moving to fixed size vectors for each shader set since there should be only one set of uniforms
    //per shader. simpler easier but will keep cpubuffers around for variable size implementation
    //later which we will sure use later for something else.
    AnythingCache cpubuffercache;
    AnythingCache programcache;
    
    AnythingCache depth_stencil_state_cache;//We cache the depthstates here so we dont create any more than neccessarry.
    //the amount of these will probably be limited so once created we will probably wont need to free as far as I can tell.
    AnythingCache gl_texturecache;//In order to properly handle deleted textures across multiple objects we offer a way to query for and tracking of ..
    //delted adn allocated textures. 
    AnythingCache gl_framebuffer_cache;
    
    memory_index default_buffer_size;
    
    DrawCallTables draw_tables;
    ResourceManagementTables resource_managment_tables;
    
    uint32_t uniform_buffer_bindkey = 0;//OPENGLEMU reserves the zero bind key for cpu bindings no one else can use this key for cpu bindings.
    
    YoyoVector currently_bound_buffers;
    YoyoVector currently_bound_frag_textures;
    
    float2 range_of_current_bound_buffers;//x start index into currentbound buffers and y is count to iterate over.
    float2 range_of_current_bound_frag_textures;//x start index into currentbound buffers and y is count to iterate over.
    
    GLTexture default_texture;
    float4 default_tex_data[4];
    
    uint32_t draw_index;
    //Mutexes
    TicketMutex texture_mutex;
    TicketMutex buffer_mutex;////NOTE(Ray):current not used    
    
    //Used to be spritebatch needs a new home
    uint32_t current_buffer_index;
	uint32_t buffer_count;
    uint32_t buffer_size;
    
    DispatchSemaphoreT semaphore;
    
    GPUBuffer buffer[3];
    MemoryArena arena[3];
    
    GPUBuffer atlas_index_buffer[3];
    MemoryArena atlas_index_arena[3];
    
    //??
    //    GPUBuffer matrix_variable_size_buffer[3];
    //    MemoryArena matrix_variable_size_arena[3];
    
    //matrix buffer add on for those who might need it.
    GPUBuffer matrix_buffer;//uniform
    MemoryArena matrix_buffer_arena;
    bool debug_out_program_change = false;
    bool debug_out_high = false;
    bool debug_out_general = true;
    bool debug_out_uniforms = false;
    bool debug_out_signpost = true;
    RenderPipelineState prev_frame_pipeline_state;
    RenderPipelineState default_pipeline_state;
    Texture default_depth_stencil_texture;
        
    YoyoVector temp_deleted_tex_entries;
    uint64_t glemu_tex_id;
    
    //Need to initialize these.
    RenderPassDescriptor default_render_pass_descriptor;
    RenderPassBuffer pass_buffer;

    u64 gpu_ids = 0;
    u64 framebuffer_ids = 0;    
};

void ogle_verify_com_buf(OpenGLEmuState* s,GLEMUBufferState state);    
VertexDescriptor ogle_create_default_vert_desc();    
DepthStencilDescription ogle_create_def_depth_stencil_desc();    
DepthStencilState ogle_create_def_depth_state(OpenGLEmuState* s);    
GLProgram* ogle_get_prog_ptr(OpenGLEmuState*s,GLProgramKey key);
GLProgram ogle_add_prog_source(OpenGLEmuState* s,const char* v_s,const char* vs_name,const char* f_s,const char* fs_name,VertexDescriptor vd);
GLProgram ogle_add_prog_lib(OpenGLEmuState*s,const char* vs_name,const char* fs_name,VertexDescriptor vd);    
SamplerDescriptor ogle_get_def_sampler_desc(OpenGLEmuState* s);    
void ogle_create_cpu_buf_bind(OpenGLEmuState*s,uint64_t bindkey,memory_index size);    
GLTexture ogle_tex_image_2d_with_sampler(OpenGLEmuState*s,void* texels,float2 dim,PixelFormat format,SamplerDescriptor sd,TextureUsage usage);    
GLTexture ogle_tex_image_2d(OpenGLEmuState*s,void* texels,float2 dim,PixelFormat format,TextureUsage usage);
void ogle_api_init(OpenGLEmuState* s);    
void ogle_init(OpenGLEmuState* s);    
bool ogle_is_valid_texture(OpenGLEmuState*s,GLTexture texture);    
void ogle_purge_textures(OpenGLEmuState* s);    
SamplerState ogle_get_sample_state(SamplerDescriptor desc);
UniformBindingTableEntry ogle_get_uniform_entry_for_draw(OpenGLEmuState*s,uint32_t index);
SamplerDescriptor ogle_get_sampler_desc(OpenGLEmuState*s);    
SamplerState ogle_get_def_sampler(OpenGLEmuState*s);    
//Depth and Stencil
DepthStencilDescription ogle_get_def_depth_Sten_desc(OpenGLEmuState* s);    
DepthStencilState ogle_get_depth_sten_state(OpenGLEmuState*s,DepthStencilDescription desc);
//framebuffer/Rendertargets
u64 ogle_gen_framebuffer(OpenGLEmuState* s);
void ogle_fb_tex2d_color(OpenGLEmuState* s,u64 id,u32 index,GLTexture texture,u32 mipmap_level,LoadAction l = LoadActionLoad,StoreAction st = StoreActionStore);
void ogle_fb_tex2d_depth(OpenGLEmuState* s,u64 id,GLTexture texture,u32 mipmap_level,LoadAction l = LoadActionLoad,StoreAction st = StoreActionStore);
void ogle_fb_tex2d_stencil(OpenGLEmuState* s,u64 id,GLTexture texture,u32 mipmap_level,LoadAction l = LoadActionLoad,StoreAction st = StoreActionStore);
void ogle_bind_framebuffer(OpenGLEmuState* s,u64 id);

//Buffers
GPUBuffer ogle_gen_buffer(OpenGLEmuState*s,u64 size,ResourceOptions options);
void ogle_buffer_data_named(OpenGLEmuState* s,u64 size,GPUBuffer* buffer,void* data);
void ogle_create_buffer(OpenGLEmuState*s,uint64_t bindkey);
void ogle_add_buffer_binding(OpenGLEmuState*s,uint64_t buffer_key,uint64_t key);

TripleGPUBuffer* ogle_get_buffer_binding(OpenGLEmuState*s,uint64_t bindkey);    
YoyoVector ogle_get_buf_list(OpenGLEmuState*s);    
YoyoVector ogle_get_program_list(OpenGLEmuState*s);    
YoyoVector ogle_get_texture_list(OpenGLEmuState*s);
void ogle_bind_texture_frag(OpenGLEmuState*s,GLTexture texture,uint32_t tex_index);
void ogle_bind_texture_frag_sampler(OpenGLEmuState*s,GLTexture texture,uint32_t tex_index,uint32_t sampler_index);
//TODO(Ray):Add and test this someday
/*
  void AddVertTextureBinding(Texture texture,uint32_t index)
  {
  FragmentShaderTextureBindingTableEntry entry;
  entry.index = index;
  entry.texture = texture;
  YoyoStretchPushBack(&currently_bound_frag_textures,entry);
  float2 start_count = range_of_current_bound_frag_textures;
  start_count += float2(0,1); 
  range_of_current_bound_frag_textures = start_count;
  }
*/
    
void ogle_bind_buffer(OpenGLEmuState*s,uint64_t bind_key,uint64_t index,uint64_t offset);
void ogle_bind_buffer_raw(OpenGLEmuState* s,u64 bk,u64 i,u64 o);
void ogle_create_buffer(OpenGLEmuState*s,uint64_t bindkey,memory_index size);    
void ogle_bind_cpubuffer(OpenGLEmuState* s,uint64_t buffer_key,uint64_t key);    
CPUBuffer* ogle_cpubuffer_at_binding(OpenGLEmuState*s,uint64_t bindkey);    
YoyoVector ogle_get_cpubuffer_list(OpenGLEmuState*s);
UniformBindResult ogle_add_uniform_data_at_binding(OpenGLEmuState*s,uint64_t bindkey,void* uniform_data,memory_index size);    
BufferOffsetResult ogle_get_uniform_at_binding(OpenGLEmuState*s,uint64_t bindkey,uint32_t index);
GLProgram ogle_get_def_program(OpenGLEmuState*s);    
GLProgram ogle_get_program(OpenGLEmuState*s,GLProgramKey key);    
uint32_t ogle_get_depth_sten_state_count(OpenGLEmuState*s);    
DepthStencilDescription ogle_get_current_depth_sten_state(OpenGLEmuState*s);    
uint32_t ogle_get_current_stencil_ref_value(OpenGLEmuState*s);    
UniformBindResult ogle_add_uniform_data(OpenGLEmuState* s,uint32_t buffer_binding,uint32_t data_index,GLProgram* p,uint32_t size);
void* ogle_frag_set_uniform_(OpenGLEmuState*s,memory_index size,u32 index);    
void* ogle_vert_set_uniform_(OpenGLEmuState*s,memory_index size,u32 index);    
uint32_t ogle_add_draw_call_entry(OpenGLEmuState*s,BufferOffsetResult v_uni_bind,BufferOffsetResult f_uni_bind,BufferOffsetResult tex_binds,u32 v_uni_bind_index,u32 f_uni_bind_index);    
void ogle_end_draw(OpenGLEmuState*s,uint32_t unit_size);    
void ogle_pre_frame_setup(OpenGLEmuState*s);
void ogle_blend(OpenGLEmuState*s,uint64_t gl_src,uint64_t gl_dst);
void ogle_blend_i(OpenGLEmuState*s,u32 i,uint64_t gl_src,uint64_t gl_dst);    
void ogle_use_program(OpenGLEmuState*s,GLProgram gl_program);    
void ogle_enable_scissor_test(OpenGLEmuState*s);    
void ogle_disable_scissor_test(OpenGLEmuState*s);    
void ogle_scissor_test(OpenGLEmuState*s,int x,int y,int width,int height);    
void ogle_scissor_test_f4(OpenGLEmuState*s,float4 rect);    
void ogle_viewport(OpenGLEmuState*s,int x, int y, int width, int height);
void ogle_viewport_f4(OpenGLEmuState*s,float4 vp);    
void ogl_bind_framebuffer_start(OpenGLEmuState*s,GLTexture texture);    
void ogle_bind_framebuffer_end(OpenGLEmuState*s);

//depth
void ogle_enable_depth_test(OpenGLEmuState*s);
void ogle_disable_depth_test(OpenGLEmuState*s);
void ogle_depth_func(OpenGLEmuState* s,CompareFunc func);

//stencil
void ogle_enable_stencil_test(OpenGLEmuState*s);    
void ogle_disable_stencil_test(OpenGLEmuState*s);    
void ogle_stencil_mask(OpenGLEmuState*s,uint32_t mask);    
void ogle_stencil_mask_sep(OpenGLEmuState*s,uint32_t front_or_back,uint32_t mask);    
void ogle_stencil_func(OpenGLEmuState*s,CompareFunc func,uint32_t ref,uint32_t mask);    
void ogle_stencil_func_sep(OpenGLEmuState*s,uint32_t front_or_back,CompareFunc func,uint32_t ref,uint32_t mask);
void ogle_stencil_op(OpenGLEmuState*s,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass);
void ogle_stencil_op_sep(OpenGLEmuState*s,uint32_t front_or_back,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass);    
void ogle_stencil_func_and_op(OpenGLEmuState*s,CompareFunc func,uint32_t ref,uint32_t mask,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass);    
void ogle_stencil_Func_and_op_sep(OpenGLEmuState*s,uint32_t front_or_back,CompareFunc func,uint32_t ref,uint32_t mask,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass);    
void ogle_clear_buffer(OpenGLEmuState*s,uint32_t buffer_bits);
void ogle_clear_color_target(OpenGLEmuState*s,uint32_t index);
void ogle_clear_depth(OpenGLEmuState*s,uint32_t value);    
void ogle_clear_stencil(OpenGLEmuState*s,uint32_t value);    
void ogle_clear_color(OpenGLEmuState*s,float4 value);    
void ogle_clear_color_and_stencil(OpenGLEmuState*s,float4 color,uint32_t stencil);    
//Debug calls
void ogle_add_debug_signpost(OpenGLEmuState*s,char* str);    
//NOTE(Ray):We now will reference the draw table when we add a draw and when we dispatch one
//We grab an table entry to get all the relevant uniform data for draw call and all the
//other info like texture bindings etc... for now at the time of writing this comment we only need to
//keep uniforms and maybe texture bindings to complete the current project.
//TODO(Ray):These will clean up nice once we get uniform variable support
void ogle_draw_arrays(OpenGLEmuState*s,uint32_t current_count,uint32_t unit_size);
void ogle_draw_array_primitives(OpenGLEmuState*s,uint32_t current_count,uint32_t unit_size);
void ogle_draw_elements(OpenGLEmuState*s,uint32_t count,uint32_t index_type,u64 element_buffer_id,u64 element_offset);

void ogle_execute(OpenGLEmuState*s,void* pass_in_c_buffer,bool enqueue,bool commit,bool present);
void ogle_execute_commit(OpenGLEmuState*s,void* pass_in_c_buffer,bool present);
void ogle_execute_enqueue(OpenGLEmuState*s,void* pass_in_c_buffer);
void ogle_execute_passthrough(OpenGLEmuState*s,void* pass_in_c_buffer);

#define GLEMU_STATE
#endif

#ifdef YOYOIMPL    
void ogle_verify_com_buf(OpenGLEmuState* s,GLEMUBufferState state) 
{
    u32 current_command_index = 0;
    uint32_t index = 0;
    while (index < s->command_list.count)
    {
        GLEMUCommandHeader* header = (GLEMUCommandHeader*)s->command_list.buffer.base;
        GLEMUBufferState command_type = header->type;
        if(index == s->command_list.count)
        {
            Assert(state == header->type);                
        }
        ++index;
    }
}
    
VertexDescriptor ogle_create_default_vert_desc()
{
    VertexDescriptor vertex_descriptor = RenderEncoderCode::NewVertexDescriptor();
    //vertesx descriptors
    VertexAttributeDescriptor vad;
    vad.format = VertexFormatFloat3;
    vad.offset = 0;
    vad.buffer_index = 0;
    RenderEncoderCode::AddVertexAttribute(&vertex_descriptor,vad);
    VertexAttributeDescriptor vcad;
    vcad.format = VertexFormatFloat4;
    vcad.offset = float3::size();
    vcad.buffer_index = 0;
    RenderEncoderCode::AddVertexAttribute(&vertex_descriptor,vcad);
    VertexAttributeDescriptor uv_ad;
    uv_ad.format = VertexFormatFloat2;
    uv_ad.offset = float3::size() + float4::size();
    uv_ad.buffer_index = 0;
    RenderEncoderCode::AddVertexAttribute(&vertex_descriptor,uv_ad);
    //vertex layouts
    VertexBufferLayoutDescriptor vbld;
    vbld.step_function = step_function_per_vertex;
    vbld.step_rate = 1;
    vbld.stride = float3::size() + float4::size() + float2::size();
    RenderEncoderCode::AddVertexLayout(&vertex_descriptor,vbld);
    return vertex_descriptor;
}
    
DepthStencilDescription ogle_create_def_depth_stencil_desc()
{
    DepthStencilDescription depth_desc = RendererCode::CreateDepthStencilDescriptor();
    depth_desc.depthWriteEnabled = false;
    depth_desc.depthCompareFunction = compare_func_always;
    return depth_desc;
}
    
DepthStencilState ogle_create_def_depth_state(OpenGLEmuState* s)
{
    Assert(s);
    DepthStencilState depth_state = RendererCode::NewDepthStencilStateWithDescriptor(&s->default_depth_stencil_description);
    AnythingCacheCode::AddThing(&s->depth_stencil_state_cache,&s->default_depth_stencil_description,&depth_state);
    return depth_state;    
}
    
GLProgram* ogle_get_prog_ptr(OpenGLEmuState*s,GLProgramKey key)
{
    return GetThingPtr(&s->programcache,&key,GLProgram);
}

GLProgram ogle_add_prog_source(OpenGLEmuState* s,const char* v_s,const char* vs_name,const char* f_s,const char* fs_name,VertexDescriptor vd)
{
    RenderShader shader = {};
    RenderShaderCode::InitShader(&shader,(char*)v_s,(char*)vs_name,(char*)f_s,(char*)fs_name);
    GLProgram result= {};
    result.shader = shader;
    result.vd = vd;
    result.last_fragment_buffer_binding = s->uniform_buffer_bindkey;
    result.last_fragment_data_index = 0;
    result.last_vertex_buffer_binding = s->uniform_buffer_bindkey;
    result.last_vertex_data_index = 0;
    GLProgramKey program_hash_key = {(uint64_t)shader.vs_object,(uint64_t)shader.ps_object};
    AnythingCacheCode::AddThing(&s->programcache,(void*)&program_hash_key,&result);
    GLProgram* p = ogle_get_prog_ptr(s,program_hash_key);
    return result;        
}

GLProgram ogle_add_prog_lib(OpenGLEmuState*s,const char* vs_name,const char* fs_name,VertexDescriptor vd)
{
    RenderShader shader = {};
    RenderShaderCode::InitShaderFromDefaultLib(&shader,vs_name,fs_name);
    GLProgram result;
    result.shader = shader;
    result.vd = vd;
    result.last_fragment_buffer_binding = s->uniform_buffer_bindkey;
    result.last_fragment_data_index = 0;
    result.last_vertex_buffer_binding = s->uniform_buffer_bindkey;
    result.last_vertex_data_index = 0;

    GLProgram* p;
    GLProgramKey program_hash_key = {(uint64_t)shader.vs_object,(uint64_t)shader.ps_object};
    if(!AnythingCacheCode::DoesThingExist(&s->programcache,(void*)&program_hash_key))
    {
        AnythingCacheCode::AddThing(&s->programcache,(void*)&program_hash_key,&result);
    }
    else
    {
        p = (GLProgram*)AnythingCacheCode::GetThing(&s->programcache,(void*)&program_hash_key);
        Assert(p);
    }
    p = ogle_get_prog_ptr(s,program_hash_key);
    return result;
}
    
SamplerDescriptor ogle_get_def_sampler_desc(OpenGLEmuState* s)
{
    return s->defaults;
}
    
//NOTE(Ray)IMPORTANT:This must be used in a thread safe only section of code
//Actually this should only be used in one place that I can think of.  Kind of a dumb
//function tbh
inline uint64_t ogle_next_tex_id(OpenGLEmuState*s)
{
    return ++s->glemu_tex_id;
}

//CPU Only buffers mainly for uniforms
void ogle_create_cpu_buf_bind(OpenGLEmuState*s,uint64_t bindkey,memory_index size)
{
    //Start size will be something basic like 10k but resize at the end a
    CPUBuffer buffer = {};
    buffer.buffer = PlatformAllocatePartition(size);
    buffer.ranges = YoyoInitVectorSize(1,float2::size(),false);
    uint64_t tt = bindkey;
    AnythingCacheCode::AddThing(&s->cpubuffercache,(void*)&tt,&buffer);
}
    
GLTexture ogle_tex_image_2d_with_sampler(OpenGLEmuState*s,void* texels,float2 dim,PixelFormat format,SamplerDescriptor sd,TextureUsage usage)
{
    BeginTicketMutex(&s->texture_mutex);

    GLTexture texture = {};
    texture.sampler = ogle_get_def_sampler(s);
    TextureDescriptor td = {};
    td = RendererCode::Texture2DDescriptorWithPixelFormat(format,dim.x(),dim.y(),false);
#if OSX
    td.storageMode = StorageModeManaged;
    if(format == PixelFormatDepth32Float_Stencil8 || format == PixelFormatX32_Stencil8 || format == PixelFormatX24_Stencil8)
    {
        td.storageMode = StorageModePrivate;
    }
#endif
    td.usage = (TextureUsage)usage;
    texture.texture = RendererCode::NewTextureWithDescriptor(td);
    //NOTE(Ray):This is ok as long as we are not releasing any descriptors but as soon as we did...
    //we have to make sure this is a valid sampler that we add here and not one scheduled for deletion.
    texture.sampler = ogle_get_sample_state(sd);
    if(texels)
    {
        RenderRegion region;
        region.origin = float3(0);
        region.size = dim;
            
        //TODO(Ray):We will need a more comprehensive way to check that we are passing in the proper parameters
        //to Replace REgions for packed and compressed formats and proper bytes sizes to multiply width and
        //for 1d arrays should pass in 0
        int byte_size_of_format = 4;
        if(format == PixelFormatABGR4Unorm)
            byte_size_of_format = 2;
                
        RenderGPUMemory::ReplaceRegion(texture.texture,region,0,texels,byte_size_of_format * dim.x());
    }
        
    texture.gen_thread = YoyoGetThreadID();
    texture.id = ogle_next_tex_id(s);

    GLTextureKey k = {};
    k.format = texture.texture.descriptor.pixelFormat;
    k.width = texture.texture.descriptor.width;
    k.height = texture.texture.descriptor.height;
    k.sample_count = texture.texture.descriptor.sampleCount;
    k.storage_mode = texture.texture.descriptor.storageMode;
    k.allowGPUOptimizedContents = texture.texture.descriptor.allowGPUOptimizedContents;
    k.gl_tex_id = texture.id;
        
    texture.texture.is_released = false;
    texture.is_released = false;
        
    if(!AnythingCacheCode::AddThingFL(&s->gl_texturecache,&k,&texture))
    {
        PlatformOutput(true,"Texture already Exist was not added to texture cache \n");
    }
    PlatformOutput(true,"TextureCreated id %d generated on thread %d on thread %d \n",texture.id,texture.gen_thread);
        
    EndTicketMutex(&s->texture_mutex);
    return texture;
}
    
GLTexture ogle_tex_image_2d(OpenGLEmuState*s,void* texels,float2 dim,PixelFormat format,TextureUsage usage)
{
    SamplerDescriptor sd = ogle_get_def_sampler_desc(s);
    sd.r_address_mode = SamplerAddressModeClampToEdge;
    sd.s_address_mode = SamplerAddressModeClampToEdge;
    sd.min_filter = SamplerMinMagFilterLinear;
    sd.mag_filter = SamplerMinMagFilterLinear;
    return ogle_tex_image_2d_with_sampler(s,texels,dim,format,sd,usage);
}
    
void ogle_api_init(OpenGLEmuState* s)
{
    s->command_list.buffer = PlatformAllocatePartition(MegaBytes(20));
    s->is_stencil_enabled = false;
    float2 dim = RendererCode::dim;
        
    TextureDescriptor depth_texture_desc = RendererCode::Texture2DDescriptorWithPixelFormat(PixelFormatDepth32Float_Stencil8,dim.x(),dim.y(),false);
    depth_texture_desc.usage       = TextureUsageRenderTarget;
    depth_texture_desc.storageMode = StorageModePrivate;
    depth_texture_desc.pixelFormat = PixelFormatDepth32Float_Stencil8;
        
    Texture depth_texture = RendererCode::NewTextureWithDescriptor(depth_texture_desc);
    RenderPassDescriptor sp_rp_desc = RenderEncoderCode::NewRenderPassDescriptor();
    //depth attachment description
    sp_rp_desc.depth_attachment.description.texture       = depth_texture;
    sp_rp_desc.depth_attachment.description.loadAction    = LoadActionClear;
    sp_rp_desc.depth_attachment.description.storeAction   = StoreActionStore;
    sp_rp_desc.depth_attachment.clear_depth               = 1.0f;
        
    //stencil attachment description
    sp_rp_desc.stencil_attachment.description.texture     = depth_texture;
    sp_rp_desc.stencil_attachment.description.loadAction  = LoadActionClear;
    sp_rp_desc.stencil_attachment.description.storeAction = StoreActionStore;
    sp_rp_desc.stencil_attachment.clearStencil = 0.0f;
        
    sp_rp_desc.depth_attachment = sp_rp_desc.depth_attachment;
    sp_rp_desc.stencil_attachment = sp_rp_desc.stencil_attachment;
    RendererCode::SetRenderPassDescriptor(&sp_rp_desc);
        
    RenderPassColorAttachmentDescriptor sprite_rp_ca_desc = {};
    sprite_rp_ca_desc.clear_color = float4(0.392f,0.584f,0.929f,1);//corflower blue of course
    sprite_rp_ca_desc.description.loadAction = LoadActionLoad;
    sprite_rp_ca_desc.description.storeAction = StoreActionStore;

    RenderEncoderCode::AddRenderPassColorAttachment(&sp_rp_desc,&sprite_rp_ca_desc);
    
    RenderEncoderCode::SetRenderPassColorAttachmentDescriptor(&sp_rp_desc,0);
        
    s->default_render_pass_descriptor = sp_rp_desc;
        
    RenderMaterial material = SpriteBatchCode::CreateSpriteBatchMaterials("spritebatch_vs_matrix_indexed","spritebatch_fs_single_sampler","Test matrix buffer");
        
    s->default_pipeline_state = material.pipeline_state;
    for(int i = 0;i < s->buffer_count;++i)
    {
        uint32_t size = (uint32_t)s->buffer_size;
        s->buffer[i] = RenderGPUMemory::NewBufferWithLength(size,ResourceStorageModeShared);
        s->arena[i] = AllocatePartition(size,s->buffer[i].data);
            
        uint32_t atlas_index_buffer_size = (size / SIZE_OF_SPRITE_IN_BYTES) * sizeof(uint32_t);
        s->atlas_index_buffer[i] = RenderGPUMemory::NewBufferWithLength(atlas_index_buffer_size,ResourceStorageModeShared);
        s->atlas_index_arena[i] = AllocatePartition(atlas_index_buffer_size,s->atlas_index_buffer[i].data);
    }
        
    uint32_t size = (uint32_t)s->buffer_size;
    s->matrix_buffer = RenderGPUMemory::NewBufferWithLength(size,ResourceStorageModeShared);
    Assert(s->matrix_buffer.buffer);
    s->matrix_buffer_arena = AllocatePartition(size,s->matrix_buffer.data);
}
    
void ogle_init(OpenGLEmuState* s)
{
    RenderCache::Init(MAX_PSO_STATES);
        
    s->glemu_tex_id = 0;

    //TODO(Ray):Make this resizable.  GIve more control to the user over the allocation space usage
    //lifetimes without too much hassle.
//        default_buffer_size = MegaBytes(1);
    s->default_buffer_size = MegaBytes(2);
    s->buffer_count = 3;
    s->buffer_size = 1024 * SIZE_OF_SPRITE_IN_BYTES;
    s->temp_deleted_tex_entries = YoyoInitVector(1,GLTextureKey,false);
    s->semaphore = RenderSynchronization::DispatchSemaphoreCreate(s->buffer_count);

    //prime number of reasonable size for most projects to reduce collision probability
#define SIZE_OF_CACHE_TABLES 9973 
    //will need to make this configurable by the user later
#define SIZE_OF_CACHE_TABLES_SMALL 9973 /// probably large enough for most small use cache to avoid collisions
    AnythingRenderSamplerStateCache::Init(SIZE_OF_CACHE_TABLES_SMALL);
    AnythingCacheCode::Init(&s->buffercache,SIZE_OF_CACHE_TABLES_SMALL,sizeof(TripleGPUBuffer),sizeof(uint64_t));
    AnythingCacheCode::Init(&s->gpu_buffercache,SIZE_OF_CACHE_TABLES_SMALL,sizeof(GPUBuffer),sizeof(u64));    
    AnythingCacheCode::Init(&s->programcache,SIZE_OF_CACHE_TABLES,sizeof(GLProgram),sizeof(GLProgramKey));
    AnythingCacheCode::Init(&s->cpubuffercache,SIZE_OF_CACHE_TABLES,sizeof(CPUBuffer),sizeof(uint64_t));
    AnythingCacheCode::Init(&s->depth_stencil_state_cache,SIZE_OF_CACHE_TABLES,sizeof(DepthStencilState),sizeof(DepthStencilDescription));
    AnythingCacheCode::Init(&s->gl_texturecache,SIZE_OF_CACHE_TABLES,sizeof(GLTexture),sizeof(GLTextureKey),true);
    AnythingCacheCode::Init(&s->gl_framebuffer_cache,SIZE_OF_CACHE_TABLES,sizeof(GLFrameBufferInternal),sizeof(u64),true);
    
    s->samplerdescriptor = RendererCode::CreateSamplerDescriptor();
    s->defaults = s->samplerdescriptor;
        
    VertexDescriptor vd = ogle_create_default_vert_desc();
    s->default_program = ogle_add_prog_lib(s,"spritebatch_vs_matrix_indexed","spritebatch_fs_single_sampler",vd);
    s->default_shader = s->default_program.shader;
    s->default_depth_stencil_description = ogle_create_def_depth_stencil_desc();
    s->default_depth_stencil_state = ogle_create_def_depth_state(s);
        
    //create default uniform cpu buffer
    //TODO(Ray):Make user configurable ... also we dynamically make these buffers bigger at runtime.
    ogle_create_cpu_buf_bind(s,s->uniform_buffer_bindkey,MegaBytes(10));        
        
    s->draw_tables.uniform_binding_table = YoyoInitVector(1,UniformBindingTableEntry,false);
    s->draw_tables.texture_binding_table = YoyoInitVector(1,TextureBindingTableEntry,false);
    s->draw_tables.buffer_binding_table = YoyoInitVector(1,BufferBindingTableEntry,false);
        
    s->resource_managment_tables = {};
    AnythingCacheCode::Init(&s->resource_managment_tables.released_textures_table,SIZE_OF_CACHE_TABLES,sizeof(ReleasedTextureEntry),sizeof(GLTextureKey),true);
        
    s->currently_bound_buffers = YoyoInitVector(1,BufferBindingTableEntry,false);
    s->currently_bound_frag_textures = YoyoInitVector(1,FragmentShaderTextureBindingTableEntry,false);
        
    s->range_of_current_bound_buffers = float2(0.0f);
    s->range_of_current_bound_frag_textures = float2(0.0f);
        
    float2 dim = float2(2,2);
    float4 b = float4(0.0f);
    for(int i = 0;i < 4;++i)
    {
        s->default_tex_data[i] = b;                    
    }
    s->default_texture = ogle_tex_image_2d(s,&s->default_tex_data,dim,PixelFormatRGBA8Unorm,TextureUsageShaderRead);
    ogle_api_init(s);
}

void ogle_delete_texture(OpenGLEmuState*s,GLTexture* texture)
{
    BeginTicketMutex(&s->texture_mutex);
    GLTextureKey ttk = {};

    ttk.format = texture->texture.descriptor.pixelFormat;
    ttk.width = texture->texture.descriptor.width;
    ttk.height = texture->texture.descriptor.height;
    ttk.sample_count = texture->texture.descriptor.sampleCount;
    ttk.storage_mode = texture->texture.descriptor.storageMode;
    ttk.allowGPUOptimizedContents = texture->texture.descriptor.allowGPUOptimizedContents;
    ttk.gl_tex_id = texture->id;
        
    //If we are not in the texturecache cant delete it since its not a texture we know about.
    //And is not already been added to the released textures table.
    if(AnythingCacheCode::DoesThingExist(&s->gl_texturecache,&ttk))
    {
        if(!AnythingCacheCode::DoesThingExist(&s->resource_managment_tables.released_textures_table,&ttk))
        {
            GLTexture* tex = GetThingPtr(&s->gl_texturecache,&ttk,GLTexture);
            Assert(!texture->texture.is_released);
            if(!tex->texture.is_released)
            {
                Assert(!tex->texture.is_released);
                ReleasedTextureEntry rte = {};
                rte.tex_key = ttk;
                rte.delete_count = GLEMU_DEFAULT_TEXTURE_DELETE_COUNT;
                rte.current_count = 0;
                rte.thread_id = YoyoGetThreadID();
                PlatformOutput(true,"BeginDeleteTexture id %d generated on thread %d on thread %d ¥n",texture->id,texture->gen_thread,rte.thread_id);
                    
                rte.is_free = false;
//                    tex->texture.is_released = true;
                AnythingCacheCode::AddThingFL(&s->resource_managment_tables.released_textures_table,&ttk,&rte);
            }
        }
    }
    EndTicketMutex(&s->texture_mutex);
}
    
bool ogle_is_valid_texture(OpenGLEmuState*s,GLTexture texture)
{
        
    bool result = false;
    BeginTicketMutex(&s->texture_mutex);                                
    GLTextureKey ttk = {};
    ttk.format = texture.texture.descriptor.pixelFormat;
    ttk.width = texture.texture.descriptor.width;
    ttk.height = texture.texture.descriptor.height;
    ttk.sample_count = texture.texture.descriptor.sampleCount;
    ttk.storage_mode = texture.texture.descriptor.storageMode;
    ttk.allowGPUOptimizedContents = texture.texture.descriptor.allowGPUOptimizedContents;
    ttk.gl_tex_id = texture.id;
        
    if(!AnythingCacheCode::DoesThingExist(&s->gl_texturecache,&ttk))
    {
        result = false;
    }
    else if(!AnythingCacheCode::DoesThingExist(&s->resource_managment_tables.released_textures_table,&ttk))
    {
        result = true;
    }
    EndTicketMutex(&s->texture_mutex);        
    return result;        
}
    
void ogle_purge_textures(OpenGLEmuState* s)
{
    BeginTicketMutex(&s->texture_mutex);
    int pop_count = 0;
    for(int i = 0;i < s->resource_managment_tables.released_textures_table.anythings.count;++i)
    {
        ReleasedTextureEntry* rte = YoyoGetVectorElement(ReleasedTextureEntry,&s->resource_managment_tables.released_textures_table.anythings,i);
        if(rte)
        {
            ++rte->current_count;
            if(rte->current_count >= rte->delete_count  && rte->is_free == false)
            {
                if(AnythingCacheCode::DoesThingExist(&s->gl_texturecache,&rte->tex_key))
                {
                    GLTexture* tex = GetThingPtr(&s->gl_texturecache,&rte->tex_key,GLTexture);
                    if(tex->texture.descriptor.usage == 5)
                    {
                        continue;
                    }
                    Assert(tex->id == rte->tex_key.gl_tex_id);
                    Assert(tex->texture.state);
                    Assert(!tex->texture.is_released);
                    Assert(!tex->is_released);
//NOTE(RAY):There is some unknown crash inside set purgable state //TODO(Ray):Need to figure out what is causing it.
                    PlatformOutput(true,"Finalalize delete texture id %d generated on thread %d on thread %d \n",tex->id,tex->gen_thread,rte->thread_id);
                    RendererCode::ReleaseTexture(&tex->texture);
                    tex->is_released = true;
                    tex->texture.is_released = true;
                }
#if GLEMU_DEBUG
                else
                {
                    Assert(false);
                }
#endif
                YoyoStretchPushBack(&s->temp_deleted_tex_entries,rte->tex_key);
                rte->tex_key = {};
                rte->thread_id = 0;
                rte->current_count = 0;
                rte->is_free = true;
            }                
        }
    }
        
    for(int i = 0;i < s->temp_deleted_tex_entries.count;++i)
    {
        GLTextureKey* tkey = (GLTextureKey*)s->temp_deleted_tex_entries.base + i;
        AnythingCacheCode::RemoveThingFL(&s->resource_managment_tables.released_textures_table,tkey);
        AnythingCacheCode::RemoveThingFL(&s->gl_texturecache,tkey);
    }
        
    YoyoClearVector(&s->temp_deleted_tex_entries);
    EndTicketMutex(&s->texture_mutex);
}
    
SamplerState ogle_get_sample_state(SamplerDescriptor desc)
{
    SamplerState* result;
    if(AnythingRenderSamplerStateCache::DoesSamplerStateExists(&desc))
    {
        result = AnythingRenderSamplerStateCache::GetSamplerState(&desc);
    }
    else
    {
        SamplerState new_sampler_state = RenderGPUMemory::NewSamplerStateWithDescriptor(&desc);
        AnythingRenderSamplerStateCache::AddSamplerState(&desc,&new_sampler_state);
        result = AnythingRenderSamplerStateCache::GetSamplerState(&desc);
    }
    Assert(result);
    return (*result);                
}

UniformBindingTableEntry ogle_get_uniform_entry_for_draw(OpenGLEmuState*s,uint32_t index)
{
    UniformBindingTableEntry result = {};
    UniformBindingTableEntry* entry = YoyoGetVectorElement(UniformBindingTableEntry,&s->draw_tables.uniform_binding_table,index);
    if(entry)
    {
        result = *entry;
    }
    return result;
}

SamplerDescriptor ogle_get_sampler_desc(OpenGLEmuState*s)
{
    return s->samplerdescriptor;        
}
    
SamplerState ogle_get_def_sampler(OpenGLEmuState*s)
{
    s->default_sampler_state = ogle_get_sample_state(s->defaults);
    return s->default_sampler_state;        
}
    
//Depth and Stencil
DepthStencilDescription ogle_get_def_depth_Sten_desc(OpenGLEmuState* s)
{
    return s->default_depth_stencil_description;
}
    
DepthStencilState ogle_get_depth_sten_state(OpenGLEmuState*s,DepthStencilDescription desc)
{
    DepthStencilState result = {};
    if(AnythingCacheCode::DoesThingExist(&s->depth_stencil_state_cache,&desc))
    {
        result = GetThingCopy(&s->depth_stencil_state_cache,(void*)&desc,DepthStencilState);
    }
    else
    {
        result = RendererCode::NewDepthStencilStateWithDescriptor(&desc);
        result.desc = desc;
        AnythingCacheCode::AddThing(&s->depth_stencil_state_cache,(void*)&desc,&result);            
    }
        
    Assert(desc.depthCompareFunction == result.desc.depthCompareFunction);
    Assert(desc.depthWriteEnabled == result.desc.depthWriteEnabled);
    Assert(desc.backFaceStencil.stencilFailureOperation == result.desc.backFaceStencil.stencilFailureOperation);
    Assert(desc.backFaceStencil.depthFailureOperation == result.desc.backFaceStencil.depthFailureOperation);
    Assert(desc.backFaceStencil.stencilCompareFunction == result.desc.backFaceStencil.stencilCompareFunction);
    Assert(desc.backFaceStencil.write_mask == result.desc.backFaceStencil.write_mask);
    Assert(desc.backFaceStencil.read_mask == result.desc.backFaceStencil.read_mask);
    Assert(desc.backFaceStencil.enabled == result.desc.backFaceStencil.enabled);
        
    Assert(desc.frontFaceStencil.stencilFailureOperation == result.desc.frontFaceStencil.stencilFailureOperation);
    Assert(desc.frontFaceStencil.depthFailureOperation == result.desc.frontFaceStencil.depthFailureOperation);
    Assert(desc.frontFaceStencil.stencilCompareFunction == result.desc.frontFaceStencil.stencilCompareFunction);
    Assert(desc.frontFaceStencil.write_mask == result.desc.frontFaceStencil.write_mask);
    Assert(desc.frontFaceStencil.read_mask == result.desc.frontFaceStencil.read_mask);
    Assert(desc.frontFaceStencil.enabled == result.desc.frontFaceStencil.enabled);
    return result;
}
    
//Buffers

//TODO(Ray):Should we make the creation of buffers threadsafe? for now its not
inline u64 ogle_get_next_gpu_id(OpenGLEmuState* s)
{
    return ++s->gpu_ids;
}

GPUBuffer ogle_gen_buffer(OpenGLEmuState*s,u64 size,ResourceOptions options)
{
    GPUBuffer buffer = RenderGPUMemory::NewBufferWithLength(size,options);
    buffer.id = ogle_get_next_gpu_id(s);
    AnythingCacheCode::AddThing(&s->gpu_buffercache,(void*)&buffer.id,&buffer);    
    return buffer;
}

void ogle_buffer_data_named(OpenGLEmuState* s,u64 size,GPUBuffer* buffer,void* data)
{
    RenderGPUMemory::UploadBufferData(buffer,data,size);
}

void ogle_create_buffer(OpenGLEmuState*s,uint64_t bindkey)
{
    TripleGPUBuffer buffer = {};
    for(int i = 0;i < 3;++i)
    {
        uint32_t size = (uint32_t)s->default_buffer_size;
        buffer.buffer[i] = RenderGPUMemory::NewBufferWithLength(size,ResourceStorageModeShared);
        Assert(buffer.buffer[i].data);
        buffer.arena[i] = AllocatePartition(size,buffer.buffer[i].data);
        buffer.buffer[i].id = ogle_get_next_gpu_id(s);
        AnythingCacheCode::AddThing(&s->gpu_buffercache,(void*)&buffer.buffer[i].id,&buffer.buffer[i]);
    }
    uint64_t tt = bindkey;
    AnythingCacheCode::AddThing(&s->buffercache,(void*)&tt,&buffer);
}

void ogle_add_buffer_binding(OpenGLEmuState*s,uint64_t buffer_key,uint64_t key)
{
    uint64_t tt = key;
    if(!AnythingCacheCode::DoesThingExist(&s->buffercache,&tt))
    {
        uint64_t* ptr = YoyoGetElementByHash(uint64_t,&s->buffercache.hash,&buffer_key,s->buffercache.hash.key_size);
        YoyoAddElementToHashTable(&s->buffercache.hash,(void*)&tt,s->buffercache.key_size,ptr);
    }
}

TripleGPUBuffer* ogle_get_buffer_binding(OpenGLEmuState*s,uint64_t bindkey)
{
    uint64_t tt = bindkey;
    return GetThingPtr(&s->buffercache,(void*)&tt,TripleGPUBuffer);        
}

GPUBuffer* ogle_get_gpu_buffer_binding(OpenGLEmuState*s,uint64_t bindkey)
{
    uint64_t tt = bindkey;
    return GetThingPtr(&s->gpu_buffercache,(void*)&tt,GPUBuffer);        
}
    
YoyoVector ogle_get_buf_list(OpenGLEmuState*s)
{
    return s->buffercache.anythings;
}
    
YoyoVector ogle_get_program_list(OpenGLEmuState*s)
{
    return s->programcache.anythings;
}
    
YoyoVector ogle_get_texture_list(OpenGLEmuState*s)
{
    return s->gl_texturecache.anythings;
}

void ogle_bind_texture_frag_sampler(OpenGLEmuState*s,GLTexture texture,uint32_t tex_index,uint32_t sampler_index)
{
    Assert(texture.texture.state);
    Assert(texture.sampler.state);
        
    if(ogle_is_valid_texture(s,texture))
    {
    }
    else
    {
        texture = s->default_texture;
    }
        
    FragmentShaderTextureBindingTableEntry entry = {};
    entry.tex_index = tex_index;
    entry.sampler_index = sampler_index;
    entry.texture = texture;
        
    YoyoStretchPushBack(&s->currently_bound_frag_textures,entry);
    float2 start_count = s->range_of_current_bound_frag_textures;
    start_count += float2(0,1); 
    s->range_of_current_bound_frag_textures = start_count;            
}

void ogle_bind_texture_frag(OpenGLEmuState*s,GLTexture texture,uint32_t index)
{
    Assert(texture.texture.state);
    Assert(texture.sampler.state);
    if(ogle_is_valid_texture(s,texture))        
    {
    }
    else
    {
        texture = s->default_texture;    
    }
        
    FragmentShaderTextureBindingTableEntry entry = {};
    entry.tex_index = index;
    entry.sampler_index = index;
    entry.texture = texture;
        
    YoyoStretchPushBack(&s->currently_bound_frag_textures,entry);
    float2 start_count = s->range_of_current_bound_frag_textures;
    start_count += float2(0,1); 
    s->range_of_current_bound_frag_textures = start_count;
}
        
//TODO(Ray):Add and test this someday
/*
  void AddVertTextureBinding(Texture texture,uint32_t index)
  {
  FragmentShaderTextureBindingTableEntry entry;
  entry.index = index;
  entry.texture = texture;
  YoyoStretchPushBack(&currently_bound_frag_textures,entry);
  float2 start_count = range_of_current_bound_frag_textures;
  start_count += float2(0,1); 
  range_of_current_bound_frag_textures = start_count;
  }
*/
    
void ogle_bind_buffer(OpenGLEmuState*s,uint64_t bind_key,uint64_t index,uint64_t offset)
{
    BufferBindingTableEntry entry = {};
    TripleGPUBuffer* t_buffer = ogle_get_buffer_binding(s,bind_key);
    GPUBuffer buffer = t_buffer->buffer[s->current_buffer_index];
    entry.key = buffer.id;
    entry.offset = offset;//NOTE(Ray):For now we will just hold the hole buffer rather than a key
    entry.index = index;
    YoyoStretchPushBack(&s->currently_bound_buffers,entry);
    float2 start_count = s->range_of_current_bound_buffers;
    start_count += float2(0,1); 
    s->range_of_current_bound_buffers = start_count;
}

void ogle_bind_buffer_raw(OpenGLEmuState* s,u64 bind_key,u64 index,u64 offset)
{
    BufferBindingTableEntry entry = {};
    entry.key = bind_key;//Must be a gpubuffer
    entry.offset = offset;//NOTE(Ray):For now we will just hold the hole buffer rather than a key
    entry.index = index;
    YoyoStretchPushBack(&s->currently_bound_buffers,entry);
    float2 start_count = s->range_of_current_bound_buffers;
    start_count += float2(0,1); 
    s->range_of_current_bound_buffers = start_count;    
}
    
//Add a key to the list pointing to the buffer that is previously allocated.
//if the key is a duplicate no need to add it.
void ogle_bind_cpubuffer(OpenGLEmuState* s,uint64_t buffer_key,uint64_t key)
{
    uint64_t tt = key;
    if(!AnythingCacheCode::DoesThingExist(&s->cpubuffercache,&tt))
    {
        uint64_t* ptr = YoyoGetElementByHash(uint64_t,&s->cpubuffercache.hash,&buffer_key,s->cpubuffercache.hash.key_size);
        YoyoAddElementToHashTable(&s->cpubuffercache.hash,(void*)&tt,s->cpubuffercache.key_size,ptr);
    }
}
    
CPUBuffer* ogle_cpubuffer_at_binding(OpenGLEmuState*s,uint64_t bindkey)
{
    if(s->cpubuffercache.key_size > 0)
    {
        uint64_t tt = bindkey;
        return GetThingPtr(&s->cpubuffercache,(void*)&tt,CPUBuffer);
    }
    else
    {
        return 0;
    }
}
    
YoyoVector ogle_get_cpubuffer_list(OpenGLEmuState*s)
{
    return s->cpubuffercache.anythings;
}

UniformBindResult ogle_add_uniform_data_at_binding(OpenGLEmuState*s,uint64_t bindkey,void* uniform_data,memory_index size)
{
    UniformBindResult result = {};
        
    CPUBuffer* buf = ogle_cpubuffer_at_binding(s,bindkey);
    float2 oldft = float2(0.0f);

    float2* ft = YoyoPeekVectorElement(float2,&buf->ranges);
    if(ft)
    {
        oldft = *ft;
    }

    float2 newft = float2(oldft.y(),oldft.y() + size);
    YoyoStretchPushBack(&buf->ranges,newft);
        
    partition_push_params params = DefaultPartitionParams();
    params.Flags = PartitionFlag_None;
        
    //copy uni data into buffer
    //TODO(Ray):Allow for resizing
    void* dst = (void*)PushSize(&buf->buffer, size,params);
    if(uniform_data)
        memcpy(dst,uniform_data,(uint32_t)newft.y());
        
    result.ptr = dst;
    result.data_index = buf->ranges.count - 1;
    return result;
}
    
BufferOffsetResult ogle_get_uniform_at_binding(OpenGLEmuState*s,uint64_t bindkey,uint32_t index)
{
    BufferOffsetResult result = {};
    CPUBuffer* buf = ogle_cpubuffer_at_binding(s,bindkey);
    float2* range_ptr = YoyoGetVectorElement(float2,&buf->ranges,index);
    if(range_ptr)
    {
        float2 range = *range_ptr;
        Assert(buf->buffer.used >= (uint32_t)range.y());
        Assert((uint32_t)range.y() <= buf->buffer.size);
        Assert(buf->buffer.used >= (uint32_t)range.x());
        Assert((uint32_t)range.x() <= buf->buffer.size);

        result.ptr = (void*)((uint8_t*)buf->buffer.base + (uint32_t)range.x());
        result.size = range.y() - range.x();
    }
    else
    {
        result.ptr = nullptr;
        result.size = 0;
    }
    return result;
}
//NOTE(RAY):I am not a big fan of getter methods but decided to use alot of them for now due to
//the need for thread safe operations
//Shaders
GLProgram ogle_get_def_program(OpenGLEmuState*s)
{
    return s->default_program;
}
    
GLProgram ogle_get_program(OpenGLEmuState*s,GLProgramKey key)
{
    return GetThingCopy(&s->programcache,&key,GLProgram);
}
    
uint32_t ogle_get_depth_sten_state_count(OpenGLEmuState*s)
{
    return s->depth_stencil_state_cache.anythings.count;
}
    
DepthStencilDescription ogle_get_current_depth_sten_state(OpenGLEmuState*s)
{
    return s->ds;
}
    
uint32_t ogle_get_current_stencil_ref_value(OpenGLEmuState*s)
{
    return s->current_reference_value;
}
    
UniformBindResult ogle_add_uniform_data(OpenGLEmuState* s,uint32_t buffer_binding,uint32_t data_index,GLProgram* p,uint32_t size)
{
    BufferOffsetResult last = ogle_get_uniform_at_binding(s,buffer_binding,data_index);
    return ogle_add_uniform_data_at_binding(s,buffer_binding,last.ptr,size);
}
    
//NOTE(Ray):No uniform state can be larger than 2kb limitation of the SetVertexBytes API in METAL
//NOTE(Ray):Unlike gl We provide the last known state of the uniform data to the user so they can manipulate
//it how they see fit at each state more explicitely.
void* ogle_frag_set_uniform_(OpenGLEmuState*s,memory_index size,u32 shader_index)
{
    Assert(size <= KiloBytes(4));
    GLProgramKey key = {(uint64_t)s->current_program.shader.vs_object,(uint64_t)s->current_program.shader.ps_object};
    GLProgram* p = ogle_get_prog_ptr(s,key);
    Assert(p);
    UniformBindResult result = ogle_add_uniform_data(s,p->last_fragment_buffer_binding,p->last_fragment_data_index,p,size);
    p->last_fragment_data_index = result.data_index;
    p->last_fragment_buffer_binding = p->last_fragment_buffer_binding;
    p->last_fragment_uni_index = shader_index;
    return result.ptr;
}

void* ogle_vert_set_uniform_(OpenGLEmuState*s,memory_index size,u32 shader_index)
{
    Assert(size <= KiloBytes(4));
    GLProgramKey key = {(uint64_t)s->current_program.shader.vs_object,(uint64_t)s->current_program.shader.ps_object};        
    GLProgram* p = ogle_get_prog_ptr(s,key);
    Assert(p);
    UniformBindResult result = ogle_add_uniform_data(s,p->last_vertex_buffer_binding,p->last_vertex_data_index,p,size);
    p->last_vertex_data_index = result.data_index;
    p->last_vertex_buffer_binding = p->last_vertex_buffer_binding;
    p->last_vertex_uni_index = shader_index;
    return result.ptr;
}
    
//NOTE(Ray):No sparse entries in tables.
uint32_t ogle_add_draw_call_entry(OpenGLEmuState*s,BufferOffsetResult v_uni_bind,BufferOffsetResult f_uni_bind,BufferOffsetResult tex_binds,u32 v_uni_bind_index,u32 f_uni_bind_index)
{
    UniformBindingTableEntry ue;
    ue.call_index = s->draw_index;
    ue.v_size = v_uni_bind.size;
    ue.v_data = v_uni_bind.ptr;
    ue.f_size = f_uni_bind.size;
    ue.f_data = f_uni_bind.ptr;
    ue.v_index = v_uni_bind_index;
    ue.f_index = f_uni_bind_index;
    uint32_t vf_index = YoyoStretchPushBack(&s->draw_tables.uniform_binding_table,ue);
        
    TextureBindingTableEntry tex_entry;
    tex_entry.call_index = s->draw_index;
    tex_entry.size = tex_binds.size;
    tex_entry.texture_ptr = tex_binds.ptr;
    uint32_t index = YoyoStretchPushBack(&s->draw_tables.texture_binding_table,tex_entry);
        
    s->draw_index++;
    Assert(index == vf_index);
    return index;
}
    
void ogle_end_draw(OpenGLEmuState*s,uint32_t unit_size)
{
    s->range_of_current_bound_buffers = float2(s->range_of_current_bound_buffers.y());
    s->range_of_current_bound_frag_textures = float2(s->range_of_current_bound_frag_textures.y());
    s->draw_index++;        
}
    
void ogle_pre_frame_setup(OpenGLEmuState*s)
{
    s->command_list.buffer.used = 0;
    s->command_list.count = 0;
        
    uint32_t bi =  s->current_buffer_index;
    s->arena[bi].used = 0;
    s->atlas_index_arena[bi].used = 0;
    s->matrix_buffer_arena.used = 0;
    s->draw_index = 0;
        
    //reset all uniform buffers
    CPUBuffer* ub = ogle_cpubuffer_at_binding(s,0);
    if(ub)
    {
        YoyoClearVector(&ub->ranges);
        YoyoClearVector(&ub->unit_sizes);
        ub->entry_count = 0;
        ub->buffer.used = 0;
    }
        
    //Reset shader binding back to zero
    YoyoVector pl = ogle_get_program_list(s);
    for(int i = 0;i < pl.count;++i)
    {
        GLProgram* program = (GLProgram*)pl.base + i;
        program->last_fragment_buffer_binding = 0;
        program->last_fragment_data_index = 0;
            
        program->last_vertex_buffer_binding = 0;
        program->last_vertex_data_index = 0;
        program->last_vertex_uni_index = 0;
        program->last_fragment_uni_index = 0;
    }
        
    YoyoVector bl = ogle_get_buf_list(s);
    for (int i = 0; i < bl.count; ++i)
    {
        TripleGPUBuffer* buffer;
        buffer = (TripleGPUBuffer*)bl.base + i;
        buffer->from_to = float2(0.0f);
        buffer->from_to_bytes = float2(0.0f);
        buffer->arena[bi].used = 0;
        buffer->current_count = 0;
    }
        
    YoyoClearVector(&s->draw_tables.uniform_binding_table);
    YoyoClearVector(&s->draw_tables.texture_binding_table);
    YoyoClearVector(&s->currently_bound_buffers);
    YoyoClearVector(&s->currently_bound_frag_textures);
    s->range_of_current_bound_buffers = float2(0.0f);
    s->range_of_current_bound_frag_textures = float2(0.0f);
    RenderSynchronization::DispatchSemaphoreWait(&s->semaphore,YOYO_DISPATCH_TIME_FOREVER);
        
    //IMPORTANT(RAY):This must be done after the semaphore! Wait for the gpu to signal its done before
    //we move to the next buffer
    s->current_buffer_index = (s->current_buffer_index + 1) %  s->buffer_count;
    ogle_purge_textures(s);
}
    
//Commands    
inline void ogle_add_header(OpenGLEmuState*s, GLEMUBufferState type)
{
    GLEMUCommandHeader* header = PushStruct(&s->command_list.buffer,GLEMUCommandHeader);
    header->type = type;
}
    
#define OGLE_ADD_COMMAND(s,type) (type*)ogle_add_command_(s,sizeof(type));
inline void* ogle_add_command_(OpenGLEmuState*s,uint32_t size)
{
    ++s->command_list.count;
    return PushSize(&s->command_list.buffer,size);
}
    
#define OGLE_POP(ptr,type) (type*)ogle_pop_(ptr,sizeof(type));ptr = (uint8_t*)ptr + (sizeof(type));
inline void*  ogle_pop_(void* ptr,uint32_t size)
{
    return ptr;
}
    
void ogle_blend(OpenGLEmuState*s,uint64_t gl_src,uint64_t gl_dst)
{
    ogle_add_header(s,glemu_bufferstate_blend_change);
    GLEMUBlendCommand* command = OGLE_ADD_COMMAND(s,GLEMUBlendCommand);        
    uint64_t interim = gl_src;
    command->sourceRGBBlendFactor = (BlendFactor)(uint32_t)RenderGLEnum::GetMetalEnumForGLEnum(&interim);
    uint64_t interim_dest = gl_dst;
    command->destinationRGBBlendFactor = (BlendFactor)(uint32_t)RenderGLEnum::GetMetalEnumForGLEnum(&interim_dest);
}

void ogle_blend_i(OpenGLEmuState*s,u32 i,uint64_t gl_src,uint64_t gl_dst)
{
    ogle_add_header(s,glemu_bufferstate_blend_change);
    GLEMUBlendICommand* command = OGLE_ADD_COMMAND(s,GLEMUBlendICommand);        
    uint64_t interim = gl_src;
    command->sourceRGBBlendFactor = (BlendFactor)(uint32_t)RenderGLEnum::GetMetalEnumForGLEnum(&interim);
    uint64_t interim_dest = gl_dst;
    command->destinationRGBBlendFactor = (BlendFactor)(uint32_t)RenderGLEnum::GetMetalEnumForGLEnum(&interim_dest);
    command->index = i;
}
    
void ogle_use_program(OpenGLEmuState*s,GLProgram gl_program)
{
    ogle_add_header(s,glemu_bufferstate_shader_program_change);
    s->current_program = gl_program;
    GLEMUUseProgramCommand* command = OGLE_ADD_COMMAND(s,GLEMUUseProgramCommand);
    command->program = gl_program;
}
    
void ogle_enable_scissor_test(OpenGLEmuState*s)
{
    ogle_add_header(s,glemu_bufferstate_scissor_test_enable);
    GLEMUScissorTestCommand* command = OGLE_ADD_COMMAND(s,GLEMUScissorTestCommand);
    command->is_enable = true;
}
    
void ogle_disable_scissor_test(OpenGLEmuState*s)
{
    ogle_add_header(s,glemu_bufferstate_scissor_test_disable);
    GLEMUScissorTestCommand* command = OGLE_ADD_COMMAND(s,GLEMUScissorTestCommand);
    command->is_enable = false;
}
    
void ogle_scissor_test(OpenGLEmuState*s,int x,int y,int width,int height)
{
    ogle_add_header(s,glemu_bufferstate_scissor_rect_change);
    GLEMUScissorRectCommand* command = OGLE_ADD_COMMAND(s,GLEMUScissorRectCommand);
    ScissorRect s_rect = {};
    s_rect.x = x;
    s_rect.y = y;
    s_rect.width = width;
    s_rect.height = height;
    command->s_rect = s_rect;
}
    
void ogle_scissor_test_f4(OpenGLEmuState*s,float4 rect)
{
    ogle_scissor_test(s,rect.x(),rect.y(),rect.z(),rect.w());
}
    
void ogle_viewport(OpenGLEmuState*s,int x, int y, int width, int height)
{
    ogle_add_header(s,glemu_bufferstate_viewport_change);
    GLEMUViewportChangeCommand* command = OGLE_ADD_COMMAND(s,GLEMUViewportChangeCommand);
    command->viewport = float4(x,y,width,height);
}

void ogle_viewport_f4(OpenGLEmuState*s,float4 vp)
{
    ogle_viewport(s,vp.x(),vp.y(),vp.z(),vp.w());
}

u64 ogle_gen_framebuffer(OpenGLEmuState* s)
{
    GLFrameBufferInternal newfb = {};
    u64 id = ++s->framebuffer_ids;
    newfb.id = id;
    YoyoBufferFixed fb = BufInit(4,sizeof(RenderPassColorAttachmentDescriptor));
    newfb.color = fb;
    for(int i = 0;i < 4;++i)
    {
        RenderPassColorAttachmentDescriptor rad = {};
        PushBuf(&newfb.color,&rad);
    }
    AnythingCacheCode::AddThingFL(&s->gl_framebuffer_cache,(void*)&id ,&newfb);
    return id;
}

void ogle_fb_tex2d_color(OpenGLEmuState* s,u64 id,u32 index,GLTexture texture,u32 mipmap_level,LoadAction l,StoreAction st)
{
    //NOTE(Ray):Since on apple hardware the absolute minimum spec for metal is 4 we will use that as
    //our low value
    //TODO(Ray):Get the max number of color attachements allowed from the hardware or tier.
    Assert(index < 4);
    if(AnythingCacheCode::DoesThingExist(&s->gl_framebuffer_cache,(void*)&id))
    {
        GLFrameBufferInternal* fb = GetThingPtr(&s->gl_framebuffer_cache,(void*)&id,GLFrameBufferInternal);
        RenderPassColorAttachmentDescriptor* cd = (RenderPassColorAttachmentDescriptor*)GetBufEle(&fb->color,index);
        RenderPassAttachmentDescriptor rad = {};
        rad.texture = texture.texture;
        rad.level = mipmap_level;
        rad.loadAction = l;
        rad.storeAction = st;
        rad.storeActionOptions = StoreActionOptionNone;
        cd->clear_color = float4(0.0f);
        cd->description = rad;
    }
    else
    {
//Your framebuffer is invalid or does not exist.
        Assert(false);
    }
}

void ogle_fb_tex2d_depth(OpenGLEmuState* s,u64 id,GLTexture texture,u32 mipmap_level,LoadAction l,StoreAction st)
{
    u64 sid = id;
    if(AnythingCacheCode::DoesThingExist(&s->gl_framebuffer_cache,(void*)&sid))
    {
        GLFrameBufferInternal* fb = GetThingPtr(&s->gl_framebuffer_cache,(void*)&id,GLFrameBufferInternal);
        RenderPassDepthAttachmentDescriptor* dad = &fb->depth;
        RenderPassAttachmentDescriptor rad = {};

        rad.texture = texture.texture;
        rad.level = mipmap_level;
        rad.loadAction = l;
        rad.storeAction = st;
        rad.storeActionOptions = StoreActionOptionNone;
        dad->clear_depth = 1.0f;
        dad->resolve_filter =     MultisampleDepthResolveFilterSample0;
        dad->description = rad;
    }
    else
    {
//Your framebuffer is invalid or does not exist.
        Assert(false);
    }    
}

void ogle_fb_tex2d_stencil(OpenGLEmuState* s,u64 id,GLTexture texture,u32 mipmap_level,LoadAction l,StoreAction st)
{
    if(AnythingCacheCode::DoesThingExist(&s->gl_framebuffer_cache,(void*)&id))
    {
        GLFrameBufferInternal* fb = GetThingPtr(&s->gl_framebuffer_cache,(void*)&id,GLFrameBufferInternal);
        RenderPassStencilAttachmentDescriptor* sad = &fb->stencil;
        RenderPassAttachmentDescriptor rad = {};
        rad.texture = texture.texture;
        rad.level = mipmap_level;
        rad.loadAction = l;
        rad.storeAction = st;
        rad.storeActionOptions = StoreActionOptionNone;
        sad->clearStencil = 1.0f;
        sad->stencilResolveFilter = 0.0f;
        sad->description = rad;
    }
    else
    {
//Your framebuffer is invalid or does not exist.
        Assert(false);
    }        
}

void ogle_bind_framebuffer(OpenGLEmuState* s,u64 id)
{
    ogle_add_header(s,glemu_bufferstate_framebuffer_bind);
    GLEMUBindFrameBufferCommand* command = OGLE_ADD_COMMAND(s,GLEMUBindFrameBufferCommand);
    command->id = id;    
}

void ogl_bind_framebuffer_start(OpenGLEmuState*s,GLTexture texture)
{
    Texture texture_for_framebuffer = texture.texture;
    if(texture_for_framebuffer.state != nullptr)
    {
        ogle_add_header(s,glemu_bufferstate_start);
        GLEMUFramebufferStart* command = OGLE_ADD_COMMAND(s,GLEMUFramebufferStart);
        command->texture = texture_for_framebuffer;
    }
}
    
void ogle_bind_framebuffer_end(OpenGLEmuState*s)
{ 
    ogle_add_header(s,glemu_bufferstate_end);
    GLEMUFramebufferEnd* command = OGLE_ADD_COMMAND(s,GLEMUFramebufferEnd);       
}

//depth
void ogle_enable_depth_test(OpenGLEmuState*s)
{
    ogle_add_header(s,glemu_bufferstate_depth_enable);
    GLEMUDepthStateCommand* command = OGLE_ADD_COMMAND(s,GLEMUDepthStateCommand);
    command->is_enable = true;        
}
    
void ogle_disable_depth_test(OpenGLEmuState*s)
{
    ogle_add_header(s,glemu_bufferstate_depth_disable);
    GLEMUDepthStateCommand* command = OGLE_ADD_COMMAND(s,GLEMUDepthStateCommand);
    command->is_enable = false;
}
    
void ogle_depth_func(OpenGLEmuState* s,CompareFunc func)
{
    ogle_add_header(s,glemu_bufferstate_depth_func);
    GLEMUDepthFuncCommand* command = OGLE_ADD_COMMAND(s,GLEMUDepthFuncCommand);
    command->func = func;
}

//stencil
void ogle_enable_stencil_test(OpenGLEmuState*s)
{
    ogle_add_header(s,glemu_bufferstate_stencil_enable);
    GLEMUStencilStateCommand* command = OGLE_ADD_COMMAND(s,GLEMUStencilStateCommand);
    command->is_enable = true;        
}
    
void ogle_disable_stencil_test(OpenGLEmuState*s)
{
    ogle_add_header(s,glemu_bufferstate_stencil_disable);
    GLEMUStencilStateCommand* command = OGLE_ADD_COMMAND(s,GLEMUStencilStateCommand);
    command->is_enable = false;
}
    
void ogle_stencil_mask(OpenGLEmuState*s,uint32_t mask)
{
    ogle_add_header(s,glemu_bufferstate_stencil_mask);
    GLEMUStencilMaskCommand* command = OGLE_ADD_COMMAND(s,GLEMUStencilMaskCommand);
    command->write_mask_value = mask;
    s->ds.frontFaceStencil.write_mask = mask;
    s->ds.backFaceStencil.write_mask = mask;
}
    
void ogle_stencil_mask_sep(OpenGLEmuState*s,uint32_t front_or_back,uint32_t mask)
{
    ogle_add_header(s,glemu_bufferstate_stencil_mask_sep);
    GLEMUStencilMaskSepCommand* command = OGLE_ADD_COMMAND(s,GLEMUStencilMaskSepCommand);
    command->write_mask_value = mask;
    command->front_or_back = front_or_back;
        
    if(front_or_back)
    {
        s->ds.frontFaceStencil.write_mask = mask;
    }
    else
    {
        s->ds.backFaceStencil.write_mask = mask;                                
    }
}
    
void ogle_stencil_func(OpenGLEmuState*s,CompareFunc func,uint32_t ref,uint32_t mask)
{
    ogle_add_header(s,glemu_bufferstate_stencil_func);
    GLEMUStencilFunCommand* command = OGLE_ADD_COMMAND(s,GLEMUStencilFunCommand);
    command->compareFunction = func;
    command->mask_value = ref;
    command->write_mask_value = mask;
        
    s->ds.frontFaceStencil.stencilCompareFunction = func;
    s->ds.frontFaceStencil.read_mask = mask;
    s->ds.backFaceStencil.stencilCompareFunction = func;
    s->ds.backFaceStencil.read_mask = mask;
        
    s->current_reference_value = ref;
}
    
void ogle_stencil_func_sep(OpenGLEmuState*s,uint32_t front_or_back,CompareFunc func,uint32_t ref,uint32_t mask)
{
    ogle_add_header(s,glemu_bufferstate_stencil_func_sep);
    GLEMUStencilFunSepCommand* command = OGLE_ADD_COMMAND(s,GLEMUStencilFunSepCommand);
    command->front_or_back = front_or_back;
    command->compareFunction = func;
    command->mask_value = ref;
    command->write_mask_value = mask;
        
    if(front_or_back)
    {
        s->ds.frontFaceStencil.stencilCompareFunction = func;
        s->ds.frontFaceStencil.read_mask = mask;
    }
    else
    {
        s->ds.backFaceStencil.stencilCompareFunction = func;
        s->ds.backFaceStencil.read_mask = mask;
    }
    s->current_reference_value = ref;
}
    
void ogle_stencil_op(OpenGLEmuState*s,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass)
{
    ogle_add_header(s,glemu_bufferstate_stencil_op);
    GLEMUStencilOpCommand* command = OGLE_ADD_COMMAND(s,GLEMUStencilOpCommand);
    command->stencil_fail_op = sten_fail;
    command->depth_fail_op = dpfail;
    command->depth_stencil_pass_op = dppass;
        
    s->ds.frontFaceStencil.stencilFailureOperation = sten_fail;
    s->ds.frontFaceStencil.depthFailureOperation = dpfail;
    s->ds.frontFaceStencil.depthStencilPassOperation = dppass;
        
    s->ds.backFaceStencil.stencilFailureOperation = sten_fail;
    s->ds.backFaceStencil.depthFailureOperation = dpfail;
    s->ds.backFaceStencil.depthStencilPassOperation = dppass;
}
    
void ogle_stencil_op_sep(OpenGLEmuState*s,uint32_t front_or_back,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass)
{
    ogle_add_header(s,glemu_bufferstate_stencil_op_sep);
    GLEMUStencilOpSepCommand* command = OGLE_ADD_COMMAND(s,GLEMUStencilOpSepCommand);
    command->front_or_back = front_or_back;
    command->stencil_fail_op = sten_fail;
    command->depth_fail_op = dpfail;
    command->depth_stencil_pass_op = dppass;
        
    if(front_or_back)
    {
        s->ds.frontFaceStencil.stencilFailureOperation = sten_fail;
        s->ds.frontFaceStencil.depthFailureOperation = dpfail;
        s->ds.frontFaceStencil.depthStencilPassOperation = dppass;
    }
    else
    {
        s->ds.backFaceStencil.stencilFailureOperation = sten_fail;
        s->ds.backFaceStencil.depthFailureOperation = dpfail;
        s->ds.backFaceStencil.depthStencilPassOperation = dppass;                                
    }
}
    
void ogle_stencil_func_and_op(OpenGLEmuState*s,CompareFunc func,uint32_t ref,uint32_t mask,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass)
{
    ogle_add_header(s,glemu_bufferstate_stencil_func_and_op);
    GLEMUStencilFuncAndOpCommand* command = OGLE_ADD_COMMAND(s,GLEMUStencilFuncAndOpCommand);
    command->compareFunction = func;
    command->write_mask_value = mask;
    command->stencil_fail_op = sten_fail;
    command->depth_fail_op = dpfail;
    command->depth_stencil_pass_op = dppass;
    command->mask_value = ref;        
        
    s->ds.frontFaceStencil.stencilCompareFunction = func;
    s->ds.frontFaceStencil.read_mask = mask;
    s->ds.backFaceStencil.stencilCompareFunction = func;
    s->ds.backFaceStencil.read_mask = mask;
        
    s->ds.frontFaceStencil.stencilFailureOperation = sten_fail;
    s->ds.frontFaceStencil.depthFailureOperation = dpfail;
    s->ds.frontFaceStencil.depthStencilPassOperation = dppass;
        
    s->ds.backFaceStencil.stencilFailureOperation = sten_fail;
    s->ds.backFaceStencil.depthFailureOperation = dpfail;
    s->ds.backFaceStencil.depthStencilPassOperation = dppass;
        
    s->current_reference_value = ref;
}
    
void ogle_stencil_Func_and_op_sep(OpenGLEmuState*s,uint32_t front_or_back,CompareFunc func,uint32_t ref,uint32_t mask,StencilOp sten_fail,StencilOp dpfail,StencilOp dppass)
{
    ogle_add_header(s,glemu_bufferstate_stencil_func_and_op_sep);
    GLEMUStencilFuncAndOpSepCommand* command = OGLE_ADD_COMMAND(s,GLEMUStencilFuncAndOpSepCommand);
    command->front_or_back = front_or_back;
    command->compareFunction = func;
    command->write_mask_value = mask;
    command->stencil_fail_op = sten_fail;
    command->depth_fail_op = dpfail;
    command->depth_stencil_pass_op = dppass;
    command->mask_value = ref;        
        
    if(front_or_back)
    {
        s->ds.frontFaceStencil.stencilCompareFunction = func;
        s->ds.frontFaceStencil.read_mask = mask;
            
        s->ds.frontFaceStencil.stencilFailureOperation = sten_fail;
        s->ds.frontFaceStencil.depthFailureOperation = dpfail;
        s->ds.frontFaceStencil.depthStencilPassOperation = dppass;
    }
    else
    {
        s->ds.backFaceStencil.stencilFailureOperation = sten_fail;
        s->ds.backFaceStencil.depthFailureOperation = dpfail;
        s->ds.backFaceStencil.depthStencilPassOperation = dppass;                                
        s->ds.backFaceStencil.stencilCompareFunction = func;
        s->ds.backFaceStencil.read_mask = mask;
    }
    s->current_reference_value = ref;
}
    
void ogle_clear_color_target(OpenGLEmuState*s,uint32_t index)
{
    ogle_add_header(s,glemu_bufferstate_clear_color_target);
    GLEMUClearColorBufferCommand* command = OGLE_ADD_COMMAND(s,GLEMUClearColorBufferCommand);
    command->index = true;
}
    
void ogle_clear_buffer(OpenGLEmuState*s,uint32_t buffer_bits)
{
    ogle_add_header(s,glemu_bufferstate_clear_start);
    GLEMUClearBufferCommand* start_command = OGLE_ADD_COMMAND(s,GLEMUClearBufferCommand);
    start_command->write_mask_value = buffer_bits;
    start_command->is_start = true;
        
    ogle_add_header(s,glemu_bufferstate_clear_end);
    GLEMUClearBufferCommand* end_command = OGLE_ADD_COMMAND(s,GLEMUClearBufferCommand);        
    end_command->is_start = false;
}

void ogle_clear_depth(OpenGLEmuState*s,uint32_t value)
{
    ogle_add_header(s,glemu_bufferstate_clear_depth_value);
    GLEMUClearDepthCommand* command = OGLE_ADD_COMMAND(s,GLEMUClearDepthCommand);        
    command->value = value;
}

void ogle_clear_stencil(OpenGLEmuState*s,uint32_t value)
{
    ogle_add_header(s,glemu_bufferstate_clear_stencil_value);
    GLEMUClearStencilCommand* command = OGLE_ADD_COMMAND(s,GLEMUClearStencilCommand);        
    command->write_mask_value = value;
}
    
void ogle_clear_color(OpenGLEmuState*s,float4 value)
{
    ogle_add_header(s,glemu_bufferstate_clear_color_value);
    GLEMUClearColorCommand* command = OGLE_ADD_COMMAND(s,GLEMUClearColorCommand);                
    command->clear_color = value;
}
    
void ogle_clear_color_and_stencil(OpenGLEmuState*s,float4 color,uint32_t stencil)
{
    ogle_add_header(s,glemu_bufferstate_clear_color_and_stencil_value);
    GLEMUClearColorAndStencilCommand* command = OGLE_ADD_COMMAND(s,GLEMUClearColorAndStencilCommand);                
    command->clear_color = color;
    command->write_mask_value = stencil;            
}
    
//Debug calls
void ogle_add_debug_signpost(OpenGLEmuState*s,char* str)
{
    ogle_add_header(s,glemu_bufferstate_debug_signpost);
    GLEMUAddDebugSignPostCommand* command = OGLE_ADD_COMMAND(s,GLEMUAddDebugSignPostCommand);                
    command->string = str;
}
    
//NOTE(Ray):We now will reference the draw table when we add a draw and when we dispatch one
//We grab an table entry to get all the relevant uniform data for draw call and all the
//other info like texture bindings etc... for now at the time of writing this comment we only need to
//keep uniforms and maybe texture bindings to complete the current project.
//@LEGACY // no longer need the unit size parameter here
void ogle_draw_arrays(OpenGLEmuState*s,uint32_t current_count,uint32_t unit_size)
{
    Assert(current_count != 0);
    ogle_add_header(s,glemu_bufferstate_draw_arrays);
    GLEMUDrawArraysCommand* command = OGLE_ADD_COMMAND(s,GLEMUDrawArraysCommand);                
    command->is_from_to = true;
    command->is_primitive_triangles = false;
    command->topology = topology_triangle;
        
    GLProgramKey key = {(uint64_t)s->current_program.shader.vs_object,(uint64_t)s->current_program.shader.ps_object};            
    GLProgram* p = ogle_get_prog_ptr(s,key);
        
    uint32_t v_buffer_binding = p->last_fragment_buffer_binding;
    uint32_t f_buffer_binding = p->last_vertex_buffer_binding;
        
    uint32_t v_data_index     = p->last_vertex_data_index;
    uint32_t f_data_index     = p->last_fragment_data_index;

    u32 v_uni_index           = p->last_vertex_uni_index;
    u32 f_uni_index           = p->last_fragment_uni_index; 
        
    BufferOffsetResult v_uni_bind_result = ogle_get_uniform_at_binding(s,v_buffer_binding,v_data_index);
    BufferOffsetResult f_uni_bind_result = ogle_get_uniform_at_binding(s,f_buffer_binding,f_data_index);
    BufferOffsetResult tex_binds = {};
    command->uniform_table_index = ogle_add_draw_call_entry(s,v_uni_bind_result,f_uni_bind_result,tex_binds,v_uni_index,f_uni_index);
    command->buffer_range = s->range_of_current_bound_buffers;
    command->texture_buffer_range = s->range_of_current_bound_frag_textures;
    command->current_count = current_count;
    ogle_end_draw(s,unit_size);
        
#if GLEMU_DEBUG
    VerifyCommandBuffer(glemu_bufferstate_draw_arrays);
#endif
}

void ogle_draw_array_primitives(OpenGLEmuState*s,uint32_t current_count,uint32_t unit_size)
{
    Assert(current_count != 0);
    ogle_add_header(s,glemu_bufferstate_draw_arrays);
    GLEMUDrawArraysCommand* command = OGLE_ADD_COMMAND(s,GLEMUDrawArraysCommand);                
    command->is_from_to = true;
    command->is_primitive_triangles = true;
    command->topology = topology_triangle;
        
    GLProgramKey key = {(uint64_t)s->current_program.shader.vs_object,(uint64_t)s->current_program.shader.ps_object};
    GLProgram p = ogle_get_program(s,key);
        
    uint32_t v_buffer_binding = p.last_fragment_buffer_binding;
    uint32_t f_buffer_binding = p.last_vertex_buffer_binding;
        
    uint32_t v_data_index     = p.last_vertex_data_index;
    uint32_t f_data_index     = p.last_fragment_data_index;

    u32 v_uni_index           = p.last_vertex_uni_index;
    u32 f_uni_index           = p.last_fragment_uni_index; 
        
    BufferOffsetResult v_uni_bind_result = ogle_get_uniform_at_binding(s,v_buffer_binding,v_data_index);
    BufferOffsetResult f_uni_bind_result = ogle_get_uniform_at_binding(s,f_buffer_binding,f_data_index);
    BufferOffsetResult tex_binds = {};
    
    command->uniform_table_index = ogle_add_draw_call_entry(s,v_uni_bind_result,f_uni_bind_result,tex_binds,v_uni_index,f_uni_index);
    command->buffer_range = s->range_of_current_bound_buffers;
    command->texture_buffer_range = s->range_of_current_bound_frag_textures;
    command->current_count = current_count;
    ogle_end_draw(s,unit_size);
        
#if GLEMU_DEBUG
    VerifyCommandBuffer(glemu_bufferstate_draw_arrays);
#endif
}

void ogle_draw_elements(OpenGLEmuState*s,uint32_t count,uint32_t index_type,u64 element_buffer_id,u64 element_offset = 0)
{
    Assert(count != 0);
    ogle_add_header(s,glemu_bufferstate_draw_elements);
    GLEMUDrawArraysCommand* command = OGLE_ADD_COMMAND(s,GLEMUDrawArraysCommand);                
    command->is_from_to = true;
    command->is_primitive_triangles = true;
    command->topology = topology_triangle;

    GLProgramKey key = {(uint64_t)s->current_program.shader.vs_object,(uint64_t)s->current_program.shader.ps_object};
    GLProgram p = ogle_get_program(s,key);
        
    uint32_t v_buffer_binding = p.last_fragment_buffer_binding;
    uint32_t f_buffer_binding = p.last_vertex_buffer_binding;
        
    uint32_t v_data_index     = p.last_vertex_data_index;
    uint32_t f_data_index     = p.last_fragment_data_index;

    u32 v_uni_index           = p.last_vertex_uni_index;
    u32 f_uni_index           = p.last_fragment_uni_index; 
        
    BufferOffsetResult v_uni_bind_result = ogle_get_uniform_at_binding(s,v_buffer_binding,v_data_index);
    BufferOffsetResult f_uni_bind_result = ogle_get_uniform_at_binding(s,f_buffer_binding,f_data_index);
    BufferOffsetResult tex_binds = {};
    
    command->uniform_table_index = ogle_add_draw_call_entry(s,v_uni_bind_result,f_uni_bind_result,tex_binds,v_uni_index,f_uni_index);
    command->buffer_range = s->range_of_current_bound_buffers;
    command->texture_buffer_range = s->range_of_current_bound_frag_textures;
    command->current_count = count;
    command->element_buffer_id = element_buffer_id;
    command->index_type = index_type;
    command->offset = element_offset;
    ogle_end_draw(s,0);
        
#if GLEMU_DEBUG
    VerifyCommandBuffer(glemu_bufferstate_draw_arrays);
#endif    
}

void ogle_execute(OpenGLEmuState*s,void* pass_in_c_buffer,bool enqueue_cb = false,bool commit_cb = true,bool present = false)
{
    void* c_buffer = nullptr;
    if(pass_in_c_buffer)
    {
        c_buffer = pass_in_c_buffer;
    }
    else
    {
        c_buffer = RenderEncoderCode::CommandBuffer();
    }
        
    //define start values for the gl render state    
    ScissorRect default_s_rect = {0,0,(int)RendererCode::dim.x(),(int)RendererCode::dim.y()};
    ScissorRect s_rect_value = default_s_rect;
    Drawable current_drawable = RenderEncoderCode::GetDefaultDrawableFromView();
        
    DepthStencilDescription current_depth_desc = ogle_get_def_depth_Sten_desc(s);
    GLProgram current_program = {};
    GLProgram default_program = {};
    TripleGPUBuffer* default_bind_buffer = ogle_get_buffer_binding(s,0);
        
    uint32_t render_encoder_count = 0;
    float4 current_clear_color = float4(0.0f);
    float current_clear_depth_value = 1.0f;
    float current_clear_stencil_value = 0.0f;
    
    if(current_drawable.state)
    {
        //Set default in_params for passes
        MatrixPassInParams in_params = {};
        in_params.s_rect = default_s_rect;
        in_params.current_drawable = current_drawable;
        in_params.viewport = float4(0,0,current_drawable.texture.descriptor.width,current_drawable.texture.descriptor.height);
        in_params.pipeline_state = s->default_pipeline_state;
            
        RenderPipelineState prev_pso = {};
        RenderPassDescriptor current_pass_desc = s->default_render_pass_descriptor;
        RenderPassDescriptor prev_pass_desc = {};
        RenderPassDescriptor last_set_pass_desc = {};
        Texture render_texture = current_drawable.texture;
        Texture current_render_texture = render_texture;
            
        bool init_params = false;
        bool was_a_clear_encoder = false;
            
        u32 current_command_index = 0;
        void* at = s->command_list.buffer.base;
            
#if METALIZER_DEBUG_OUTPUT
        PlatformOutput(s->debug_out_general, "GLEMU EXECTING COMMANDS COUNT: %d -- \n",s->command_list.count);
#endif
            
        while (current_command_index < s->command_list.count)
        {
            GLEMUCommandHeader* header = (GLEMUCommandHeader*)at;
            at = (uint8_t*)at + sizeof(GLEMUCommandHeader);
            GLEMUBufferState command_type = header->type;
            ++current_command_index;
                
            if(command_type == glemu_bufferstate_debug_signpost)
            {
                GLEMUAddDebugSignPostCommand* command = OGLE_POP(at,GLEMUAddDebugSignPostCommand);                            
#ifdef METALIZER_INSERT_DEBUGSIGNPOST
                RenderDebug::InsertDebugSignPost(in_params.re,command->string);
#endif

#if METALIZER_DEBUG_OUTPUT
                PlatformOutput(s->debug_out_general,command->string);
                PlatformOutput(s->debug_out_general,"\n");
#endif                                                
                continue;    
            }
                
            if(!init_params)
            {

                RenderPassColorAttachmentDescriptor cad = {};
                cad.description.texture = render_texture;
                
                RenderEncoderCode::SetRenderPassColorAttachmentIndex(&current_pass_desc,&cad,0);                
                current_render_texture = render_texture;                            
                render_encoder_count++;

                RenderCommandEncoder re = RenderEncoderCode::RenderCommandEncoderWithDescriptor(c_buffer,&current_pass_desc);
                
                RenderEncoderCode::SetFrontFaceWinding(&re,winding_order_counter_clockwise);
                in_params.re = re;
                last_set_pass_desc = current_pass_desc;
#if METALIZER_DEBUG_OUTPUT
                PlatformOutput(s->debug_out_general,"Setting last set_pass_desc\n");
#endif

                bool is_new_pipeline = false;
                RenderPipelineStateDesc pd = in_params.pipeline_state.desc;                
                //check consistency of pixel formats between render pass and pipeline state
                for(int i = 0;i < 4;++i)
                {
                    RenderPipelineColorAttachmentDescriptor desc = pd.color_attachments.i[i];
                    RenderPassColorAttachmentDescriptor* cd = (RenderPassColorAttachmentDescriptor*)GetBufEle(&current_pass_desc.colorAttachments,i);

                    if(cd && desc.pixelFormat != cd->description.texture.descriptor.pixelFormat)
                    {
                        desc.pixelFormat = cd->description.texture.descriptor.pixelFormat;
                        
                        //if its and someone tried to use it should just set it as a non texture
                        //and sync everything  just dont let client use invalid textures as color attachments
                        //put out a warning or silently fail?
                        if(cd->description.texture.state == nullptr)
                        {
                            desc.pixelFormat = PixelFormatInvalid;
                        }
                        
#if METALIZER_DEBUG_OUTPUT
                        PlatformOutput(s->debug_out_general,"NewPIpelineState::Due to non matching pipleline state and render pass pixel format.\n");
#endif

                        pd.color_attachments.i[i] = desc;       

                        is_new_pipeline = true;

                    }
                }
                
                //Verify and set pipeline states attachments to the same as our current renderpass
                //Do depth and stencil only for now bu tlater we want to ensure that our color attachments match as well.
                if(in_params.pipeline_state.desc.depthAttachmentPixelFormat != current_pass_desc.depth_attachment.description.texture.descriptor.pixelFormat)
                {
                    pd.depthAttachmentPixelFormat = current_pass_desc.depth_attachment.description.texture.descriptor.pixelFormat;
                    pd.stencilAttachmentPixelFormat = current_pass_desc.depth_attachment.description.texture.descriptor.pixelFormat;
                        
#if METALIZER_DEBUG_OUTPUT
                    PlatformOutput(s->debug_out_general,"NewPIpelineState::pd\n");
#endif
                    is_new_pipeline = true;
                }
                        
                //if its and someone tried to use it should just set it as a non texture
                //and sync everything  just dont let client use invalid textures as color attachments
                //put out a warning or silently fail?
                if(current_pass_desc.depth_attachment.description.texture.state == nullptr)
                {
                    pd.depthAttachmentPixelFormat = PixelFormatInvalid;
                    is_new_pipeline = true;
                }
                
                if(current_pass_desc.stencil_attachment.description.texture.state == nullptr)
                {
                    pd.stencilAttachmentPixelFormat = PixelFormatInvalid;
                    is_new_pipeline = true;
                }

                //if(is_new_pipeline)
                {
                    RenderPipelineState next_pso = RenderEncoderCode::NewRenderPipelineStateWithDescriptor(pd);
                    Assert(next_pso.desc.fragment_function);
                    Assert(next_pso.desc.sample_count == 1);
                    Assert(next_pso.desc.vertex_function);
                    Assert(next_pso.state);
                    
                    RenderEncoderCode::SetRenderPipelineState(&in_params.re,next_pso.state);
                    prev_pso = in_params.pipeline_state;
                    in_params.pipeline_state = next_pso;
                }
                        
                if(current_depth_desc.depthWriteEnabled)
                {
                    Assert((in_params.pipeline_state.desc.depthAttachmentPixelFormat != PixelFormatInvalid));                    
                }

                //NOTE(Ray):If we are scissor enable we need to clamp the scissor rect to the rendertarget surface
                if(in_params.is_s_rect)
                {
                    ScissorRect temp_rect_value = in_params.s_rect;
                    //NOTE(Ray):GL is from bottom left we are top left converting y cooridinates to match
                    temp_rect_value.y = default_s_rect.y - temp_rect_value.y;
                    //NOTE(Ray):Not allowed to specify a value outside of the current renderpass width in metal.
                    int diffw = (int)current_render_texture.descriptor.width - (temp_rect_value.x + temp_rect_value.width);
                    int diffh = (int)current_render_texture.descriptor.height - (temp_rect_value.y + temp_rect_value.height);
                    if(diffw < 0)
                        temp_rect_value.width += diffw;
                    if(diffh < 0)
                        temp_rect_value.height += diffh;
                        
                    temp_rect_value.x = clamp(temp_rect_value.x,0,current_render_texture.descriptor.width);
                    temp_rect_value.y = clamp(temp_rect_value.y,0,current_render_texture.descriptor.height);
                    temp_rect_value.width = clamp(temp_rect_value.width,0,current_render_texture.descriptor.width - temp_rect_value.x);
                    temp_rect_value.height = clamp(temp_rect_value.height,0,current_render_texture.descriptor.height - temp_rect_value.y);
                        
                    in_params.s_rect = temp_rect_value;
                    RenderEncoderCode::SetScissorRect(&in_params.re, in_params.s_rect);
                }
                init_params = true;
            }

            if(command_type == glemu_bufferstate_framebuffer_bind)
            {
                GLEMUBindFrameBufferCommand* command = OGLE_POP(at,GLEMUBindFrameBufferCommand);
                u64 fb_id = command->id;

                prev_pass_desc = last_set_pass_desc;                                                    
                RenderPassDescriptor temp_desc = last_set_pass_desc;
                
                if(fb_id != 0 && AnythingCacheCode::DoesThingExist(&s->gl_framebuffer_cache,(void*)&fb_id))
                {
                    GLFrameBufferInternal* fb = GetThingPtr(&s->gl_framebuffer_cache,(void*)&fb_id,GLFrameBufferInternal);
                    for(int i = 0;i < 4;++i)
                    {
                        RenderPassColorAttachmentDescriptor* cd = (RenderPassColorAttachmentDescriptor*)GetBufEle(&fb->color,i);
                        if(cd->description.texture.state && i == 0)
                        {
                            render_texture = cd->description.texture;
                            RenderPassColorAttachmentDescriptor* ca = (RenderPassColorAttachmentDescriptor*)GetBufEle(&temp_desc.colorAttachments,i);
                            if(ca)
                            {
                                ca->description.loadAction = cd->description.loadAction;
                                ca->description.storeAction = cd->description.storeAction;
                            }
                        }
                        else
                        {
                            //No texture for this color attachment so not used
                        }
                    }

                    //depth                    
                    Texture no_tex = {};
                    temp_desc.depth_attachment.description.texture       = fb->depth.description.texture;
                    temp_desc.depth_attachment.description.loadAction    = fb->depth.description.loadAction;
                    temp_desc.depth_attachment.description.storeAction   = fb->depth.description.storeAction;
                    temp_desc.depth_attachment.description.texture.descriptor.pixelFormat   = fb->depth.description.texture.descriptor.pixelFormat;

                    //stencil
                    temp_desc.stencil_attachment.description.texture       = fb->stencil.description.texture;
                    temp_desc.stencil_attachment.description.loadAction    = fb->stencil.description.loadAction;
                    temp_desc.stencil_attachment.description.storeAction   = fb->stencil.description.storeAction;
                    temp_desc.stencil_attachment.description.texture.descriptor.pixelFormat   = fb->stencil.description.texture.descriptor.pixelFormat;

                    current_pass_desc = temp_desc;                    
                }
                else //if we cant find the framebuffer use the default for now
                {
                    current_pass_desc = s->default_render_pass_descriptor;
                    render_texture = current_drawable.texture;
#if METALIZER_DEBUG_OUTPUT
                    PlatformOutput(s->debug_out_general,"Using default framebuffer rendertargete\n");
#endif
                }

                RenderEncoderCode::EndEncoding(&in_params.re);
                init_params = false;                                
                continue;                
            }
            
            else if(command_type == glemu_bufferstate_start)
            {
                Assert(false);
                GLEMUFramebufferStart* command = OGLE_POP(at,GLEMUFramebufferStart);
                if(command->texture.state != current_render_texture.state)
                {
                    render_texture = command->texture;
                    prev_pass_desc = last_set_pass_desc;
                        
                    RenderPassDescriptor temp_desc = last_set_pass_desc;
                    //depth attachment description
                    Texture no_tex = {};
                    temp_desc.depth_attachment.description.texture       = no_tex;
                    temp_desc.depth_attachment.description.loadAction    = LoadActionDontCare;
                    temp_desc.depth_attachment.description.storeAction   = StoreActionDontCare;
                        
                    temp_desc.stencil_attachment.description.texture       = no_tex;
                    temp_desc.stencil_attachment.description.loadAction    = LoadActionDontCare;
                    temp_desc.stencil_attachment.description.storeAction   = StoreActionDontCare;
                  
                    current_pass_desc = temp_desc;
#if METALIZER_DEBUG_OUTPUT
                    PlatformOutput(s->debug_out_general, "GLEMU BUFFERSTATE START\n");
#endif                                                
                    RenderEncoderCode::EndEncoding(&in_params.re);
                    init_params = false;                                
                }
                continue;
            }
                
            else if(command_type == glemu_bufferstate_end)
            {
                GLEMUFramebufferEnd* command = OGLE_POP(at,GLEMUFramebufferEnd);
                if(current_drawable.state != current_render_texture.state)
                {
                    Assert(prev_pass_desc.stencil_attachment.description.texture.state);
                    current_pass_desc = prev_pass_desc;
                    Assert(current_pass_desc.stencil_attachment.description.texture.state);
                    render_texture = current_drawable.texture;
                    RenderEncoderCode::EndEncoding(&in_params.re);
                        
#if METALIZER_DEBUG_OUTPUT
                    PlatformOutput(s->debug_out_general,"Framebuffer_framebuffer_end\n");
#endif

                    init_params = false;                                
                }
                continue;
            }
                
            else if(command_type == glemu_bufferstate_clear_start)
            {
                GLEMUClearBufferCommand* command = OGLE_POP(at,GLEMUClearBufferCommand);                            
                //Get buffer bits and than set the attachments to the state needed.
                //Than after the clear set them back to defaults
                if(command->write_mask_value & (1 << 1))
                {
                    RenderPassColorAttachmentDescriptor* ca = RenderEncoderCode::GetRenderPassColorAttachment(&current_pass_desc,0);
                    ca->description.loadAction = LoadActionClear;
                    ca->clear_color = current_clear_color;
                }
                    
                //TODO(Ray):Need to validate they  have actual stencil and depth textures attached if pipeline
                //has pixel format. 
                if(command->write_mask_value & (1 << 2))
                {
                    current_pass_desc.depth_attachment.description.loadAction = LoadActionClear;
                    current_pass_desc.depth_attachment.clear_depth = current_clear_depth_value;
                }

                if(command->write_mask_value & (1 << 3))
                {
                    current_pass_desc.stencil_attachment.description.loadAction = LoadActionClear;
                    current_pass_desc.stencil_attachment.clearStencil = current_clear_stencil_value;
                }
                    
                RenderEncoderCode::EndEncoding(&in_params.re);
                init_params = false;

#if METALIZER_DEBUG_OUTPUT
                PlatformOutput(s->debug_out_general, "GLEMU CLEAR START\n");
#endif
                continue;
            }
            else if(command_type == glemu_bufferstate_clear_color_target)
            {
                GLEMUClearColorBufferCommand* command = OGLE_POP(at,GLEMUClearColorBufferCommand);
                u32 index = command->index;
                //Get buffer bits and than set the attachments to the state needed.
                //Than after the clear set them back to defaults
//                if(command->write_mask_value & (1 << 1))
                {
                    RenderPassColorAttachmentDescriptor* ca = RenderEncoderCode::GetRenderPassColorAttachment(&current_pass_desc,index);
                    if(ca)
                    {
                        ca->description.loadAction = LoadActionClear;
                        ca->clear_color = current_clear_color;
                    }
                    else
                    {
#if METALIZER_DEBUG_OUTPUT
                        PlatformOutput(s->debug_out_general, "GLEMU CLEAR Did not find your rendertarget index no clear set!!\n");
#endif
                    }
                }
                continue;
            }
            else if(command_type == glemu_bufferstate_clear_end)
            {
                GLEMUClearBufferCommand* command = OGLE_POP(at,GLEMUClearBufferCommand);
                    
                //for the time being we are always in load to better emulate what opengl does.
                RenderPassColorAttachmentDescriptor* ca = RenderEncoderCode::GetRenderPassColorAttachment(&current_pass_desc,0);
                ca->description.loadAction = LoadActionLoad;
                current_pass_desc.depth_attachment.description.loadAction = LoadActionLoad;                                
                current_pass_desc.stencil_attachment.description.loadAction = LoadActionLoad;
                RenderEncoderCode::EndEncoding(&in_params.re);
                init_params = false;

#if METALIZER_DEBUG_OUTPUT
                PlatformOutput(s->debug_out_general, "GLEMU CLEAR END \n");
#endif
                continue;
            }
                
            else if(command_type == glemu_bufferstate_clear_stencil_value)
            {
                GLEMUClearStencilCommand* command = OGLE_POP(at,GLEMUClearStencilCommand);
#if METALIZER_DEBUG_OUTPUT
                PlatformOutput(s->debug_out_general, "GLEMU CLEAR stencil \n");
#endif
                current_clear_stencil_value = command->write_mask_value;
                continue;
            }
                
            else if(command_type == glemu_bufferstate_clear_depth_value)
            {
                GLEMUClearDepthCommand* command = OGLE_POP(at,GLEMUClearDepthCommand);
#if METALIZER_DEBUG_OUTPUT
                PlatformOutput(s->debug_out_general, "GLEMU CLEAR stencil' not implemented' \n");
#endif
                current_clear_depth_value = command->value;
                continue;
            }
                
            else if(command_type == glemu_bufferstate_clear_color_value)
            {
                GLEMUClearColorCommand* command = OGLE_POP(at,GLEMUClearColorCommand);                            
                current_clear_color = command->clear_color;
#if METALIZER_DEBUG_OUTPUT
                PlatformOutput(s->debug_out_general, "GLEMU CLEAR Color  \n");
#endif
                continue;
            }
                
            else if(command_type == glemu_bufferstate_clear_color_and_stencil_value)
            {
                GLEMUClearColorAndStencilCommand* command = OGLE_POP(at,GLEMUClearColorAndStencilCommand);
//Not properly implemented
                Assert(false);
#if METALIZER_DEBUG_OUTPUT
                PlatformOutput(s->debug_out_general, "GLEMU CLEAR Color and stencil command \n");
#endif
                current_clear_color = command->clear_color;
                continue;
            }
                
            else if(command_type == glemu_bufferstate_viewport_change)
            {
                GLEMUViewportChangeCommand* command = OGLE_POP(at,GLEMUViewportChangeCommand);
                    
#if METALIZER_DEBUG_OUTPUT
                PlatformOutput(s->debug_out_general, "Viewport l:%f t:%f b:%f r:%f .\n",command->viewport.x(),command->viewport.y(),command->viewport.z(),command->viewport.w());
#endif
                    
                //TODO(Ray):We need to add some checks here to keep viewport in surface bounds.
                in_params.viewport = command->viewport;
                float4 vp = in_params.viewport;
                RenderEncoderCode::SetViewport(&in_params.re,vp.x(),vp.y(),vp.z(),vp.w(),0.0f,1.0f);                            
                continue;
            }

            else if(command_type == glemu_bufferstate_blend_change_i)
            {
                GLEMUBlendICommand* command = OGLE_POP(at,GLEMUBlendICommand);
                BlendFactor source = command->sourceRGBBlendFactor;
                BlendFactor dest = command->destinationRGBBlendFactor;
                    
                RenderPipelineStateDesc pd = in_params.pipeline_state.desc;
                RenderPipelineColorAttachmentDescriptor cad = in_params.pipeline_state.desc.color_attachments.i[command->index];
                //                            if(cad.sourceRGBBlendFactor != source || cad.destinationRGBBlendFactor != dest)
                Assert((int)source >= 0);
                Assert((int)dest >= 0);
                        
                cad.sourceRGBBlendFactor = source;
                cad.sourceAlphaBlendFactor = source;
                cad.destinationRGBBlendFactor = dest;
                cad.destinationAlphaBlendFactor = dest;
                pd.color_attachments.i[command->index] = cad;
                        
                RenderPipelineState next_pso = RenderEncoderCode::NewRenderPipelineStateWithDescriptor(pd);
                Assert(next_pso.desc.fragment_function);
                Assert(next_pso.desc.sample_count == 1);
                Assert(next_pso.desc.vertex_function);
                Assert(next_pso.state);
#if METALIZER_DEBUG_OUTPUT
                PlatformOutput(s->debug_out_general,"Framebuffer_blend_change::New Pipeline State\n");
#endif
                in_params.pipeline_state = next_pso;

                RenderEncoderCode::SetRenderPipelineState(&in_params.re,in_params.pipeline_state.state);
                continue;
            }
            
            else if(command_type == glemu_bufferstate_blend_change)
            {
                GLEMUBlendCommand* command = OGLE_POP(at,GLEMUBlendCommand);
                BlendFactor source = command->sourceRGBBlendFactor;
                BlendFactor dest = command->destinationRGBBlendFactor;
                    
                RenderPipelineStateDesc pd = in_params.pipeline_state.desc;
                //NOTE(RAY):In GL if not specified with glblendfuncI(buf,sfac,dfac) than all targets get the alpha applied.
                //If this is slow for you than use ogl_blend_change_i function.
                for(int i = 0;i < 4;++i)
                {
                    RenderPipelineColorAttachmentDescriptor cad = in_params.pipeline_state.desc.color_attachments.i[i];
                    //                            if(cad.sourceRGBBlendFactor != source || cad.destinationRGBBlendFactor != dest)
                    {
                        Assert((int)source >= 0);
                        Assert((int)dest >= 0);
                        
                        cad.sourceRGBBlendFactor = source;
                        cad.sourceAlphaBlendFactor = source;
                        cad.destinationRGBBlendFactor = dest;
                        cad.destinationAlphaBlendFactor = dest;
                        pd.color_attachments.i[i] = cad;
                        
                        RenderPipelineState next_pso = RenderEncoderCode::NewRenderPipelineStateWithDescriptor(pd);
                        Assert(next_pso.desc.fragment_function);
                        Assert(next_pso.desc.sample_count == 1);
                        Assert(next_pso.desc.vertex_function);
                        Assert(next_pso.state);
#if METALIZER_DEBUG_OUTPUT
                        PlatformOutput(s->debug_out_general,"Framebuffer_blend_change::New Pipeline State\n");
#endif
                        in_params.pipeline_state = next_pso;
                    }                    
                }
                RenderEncoderCode::SetRenderPipelineState(&in_params.re,in_params.pipeline_state.state);
                continue;
            }
                
            else if(command_type == glemu_bufferstate_shader_program_change)
            {
                GLEMUUseProgramCommand* command = OGLE_POP(at,GLEMUUseProgramCommand);

//TODO(Ray):Give programs IDS and make sure we dont need to switch here if its the same program
//                    if(command->program.id != current_program.id)
                {
                    GLProgram new_program = command->program;
                    if(!new_program.shader.vs_object || !new_program.shader.ps_object)continue;
                    RenderPipelineStateDesc pd = in_params.pipeline_state.desc;
                    pd.vertex_function = new_program.shader.vs_object;
                    pd.fragment_function = new_program.shader.ps_object;
                    RenderEncoderCode::SetVertexDescriptor(&pd, &new_program.vd);
                        
                    RenderPipelineState next_pso = RenderEncoderCode::NewRenderPipelineStateWithDescriptor(pd);
                        
                    Assert(next_pso.desc.depthAttachmentPixelFormat == pd.depthAttachmentPixelFormat);
                    Assert(next_pso.desc.stencilAttachmentPixelFormat == pd.stencilAttachmentPixelFormat);
                    Assert(next_pso.desc.fragment_function);
                    Assert(next_pso.desc.sample_count == 1);
                    Assert(next_pso.desc.vertex_function);
                    Assert(next_pso.state);
#if METALIZER_DEBUG_OUTPUT                                
                    PlatformOutput(s->debug_out_program_change,"Framebuffer_shader_program_change::New Pipeline State\n");
#endif
                    current_program = new_program;
                    in_params.pipeline_state = next_pso;
                    RenderEncoderCode::SetRenderPipelineState(&in_params.re,in_params.pipeline_state.state);
                }
                continue;
            }
                
            else if(command_type == glemu_bufferstate_scissor_test_enable)
            {
                GLEMUScissorTestCommand* command = OGLE_POP(at,GLEMUScissorTestCommand);
                in_params.is_s_rect = true;

#if METALIZER_DEBUG_OUTPUT                                
                PlatformOutput(s->debug_out_general,"Framebuffer_scissor test enable\n");
#endif
                RenderEncoderCode::SetScissorRect(&in_params.re, in_params.s_rect);
                continue;
            }
                
            else if(command_type == glemu_bufferstate_scissor_test_disable)
            {
                GLEMUScissorTestCommand* command = OGLE_POP(at,GLEMUScissorTestCommand);
                in_params.is_s_rect = false;
                ScissorRect new_s_rect = {};
                new_s_rect.width = current_render_texture.descriptor.width;
                new_s_rect.height = current_render_texture.descriptor.height;
                new_s_rect.x = 0;
                new_s_rect.y = 0;

#if METALIZER_DEBUG_OUTPUT                                
                PlatformOutput(s->debug_out_general,"Framebuffer_scissor test disable\n");
#endif
                RenderEncoderCode::SetScissorRect(&in_params.re, new_s_rect);
                continue;
            }
                
            else if(command_type == glemu_bufferstate_scissor_rect_change)
            {
                GLEMUScissorRectCommand* command = OGLE_POP(at,GLEMUScissorRectCommand);
                ScissorRect temp_rect_value = command->s_rect;
                //NOTE(Ray):GL is from bottom left we are top left converting y cooridinates to match
                temp_rect_value.y = current_render_texture.descriptor.height - (temp_rect_value.height + temp_rect_value.y);
                //NOTE(Ray):Not allowed to specify a value outside of the current renderpass width in metal.
                int diffw = (int)current_render_texture.descriptor.width - (temp_rect_value.x + temp_rect_value.width);
                int diffh = (int)current_render_texture.descriptor.height - (temp_rect_value.y + temp_rect_value.height);
                if(diffw < 0)
                    temp_rect_value.width += diffw;
                if(diffh < 0)
                    temp_rect_value.height += diffh;
                    
                temp_rect_value.x = clamp(temp_rect_value.x,0,current_render_texture.descriptor.width);
                temp_rect_value.y = clamp(temp_rect_value.y,0,current_render_texture.descriptor.height);
                temp_rect_value.width = clamp(temp_rect_value.width,0,current_render_texture.descriptor.width - temp_rect_value.x);
                temp_rect_value.height = clamp(temp_rect_value.height,0,current_render_texture.descriptor.height - temp_rect_value.y);
                    
#if METALIZER_DEBUG_OUTPUT
                PlatformOutput(s->debug_out_general,"Scissor Rect Change\n");
#endif
                s_rect_value = temp_rect_value;
                in_params.s_rect = temp_rect_value;
                if(in_params.is_s_rect)
                {
                    RenderEncoderCode::SetScissorRect(&in_params.re, in_params.s_rect);                            
                }
                continue;
            }

            else if(command_type == glemu_bufferstate_depth_enable)
            {
                GLEMUDepthStateCommand* command = OGLE_POP(at,GLEMUDepthStateCommand);
                    
#ifdef METALIZER_INSERT_DEBUGSIGNPOST
                char* string = "Depth Enabled:";
                RenderDebug::InsertDebugSignPost(in_params.re,string);
#endif
#if METALIZER_DEBUG_OUTPUT
                PlatformOutput(s->debug_out_general,"Framebuffer_depth_enable\n");
#endif
                //NOTE("Must havea  depth buffer attached here");
                //The depth should? be the same size as the render target.
                //if not fragments out of the bounds of the stencil surface will get clipped.
                //Ensure we have a valid sized stencil buffer for the current render texture?
                //or leave it to the use responsibility and issue a warning.
                current_depth_desc.depthWriteEnabled = true;
                DepthStencilState state = ogle_get_depth_sten_state(s,current_depth_desc);
                RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
                //TODO(Ray):ASSERT AFTER EVERY SET DEPTHSTENCILSTATE TO ENSURE WE GOT A VALID STATE BACK!!!                            
                if(!last_set_pass_desc.depth_attachment.description.texture.state)
                {
                    //NOTE(Ray):Since we need a valid texture set for the renderpass end encoding and
                    //start a new encoder with a valid texture set on the render pass descriptor.
//                    RendererCode::SetRenderPassDescriptor(&current_pass_desc);
                    RenderEncoderCode::EndEncoding(&in_params.re);
#if METALIZER_DEBUG_OUTPUT
                    PlatformOutput(s->debug_out_general,"End Encoding Setting renderpassdesc to have texture \n");
#endif
                    init_params = false;                                                          
                }
                s->is_depth_enabled = true;
                continue;
            }
            if(command_type == glemu_bufferstate_depth_disable)
            {
                GLEMUDepthStateCommand* command = OGLE_POP(at,GLEMUDepthStateCommand);
                                    
                #ifdef METALIZER_INSERT_DEBUGSIGNPOST
                                char* string = "Depth Disabled:";
                                RenderDebug::InsertDebugSignPost(in_params.re,string);
                #endif
                #if METALIZER_DEBUG_OUTPUT
                                PlatformOutput(s->debug_out_general,"Framebuffer_depth_disable\n");
                #endif
                                //NOTE("Must havea  depth buffer attached here");
                                //The depth should? be the same size as the render target.
                                //if not fragments out of the bounds of the stencil surface will get clipped.
                                //Ensure we have a valid sized stencil buffer for the current render texture?
                                //or leave it to the use responsibility and issue a warning.
                                current_depth_desc.depthWriteEnabled = false;
                                DepthStencilState state = ogle_get_depth_sten_state(s,current_depth_desc);
                                RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
                                //TODO(Ray):ASSERT AFTER EVERY SET DEPTHSTENCILSTATE TO ENSURE WE GOT A VALID STATE BACK!!!
                                if(!last_set_pass_desc.depth_attachment.description.texture.state)
                                {
                                    //NOTE(Ray):Since we need a valid texture set for the renderpass end encoding and
                                    //start a new encoder with a valid texture set on the render pass descriptor.
                //                    RendererCode::SetRenderPassDescriptor(&current_pass_desc);
                                    RenderEncoderCode::EndEncoding(&in_params.re);
                #if METALIZER_DEBUG_OUTPUT
                                    PlatformOutput(s->debug_out_general,"End Encoding Setting renderpassdesc to have texture \n");
                #endif
                                    init_params = false;
                                }
                                s->is_depth_enabled = false;
                                continue;
            }
            else if(command_type == glemu_bufferstate_stencil_enable)
            {
                GLEMUStencilStateCommand* command = OGLE_POP(at,GLEMUStencilStateCommand);
                    
#ifdef METALIZER_INSERT_DEBUGSIGNPOST
                char* string = "Stencil Enabled:";
                RenderDebug::InsertDebugSignPost(in_params.re,string);
#endif
                //                            RenderDebug::PushDebugGroup(in_params.re,string);
#if METALIZER_DEBUG_OUTPUT
                PlatformOutput(s->debug_out_general,"Framebuffer_stencil_enable\n");
#endif
                //NOTE("Must havea  stencil buffer attached here");
                //The stencil should be the same size as the render target.
                //if not fragments out of the bounds of the stencil surface will get clipped.
                //Ensure we have a valid sized stencil buffer for the current render texture 
                current_depth_desc.backFaceStencil.enabled = true;
                current_depth_desc.frontFaceStencil.enabled = true;
                DepthStencilState state = ogle_get_depth_sten_state(s,current_depth_desc);
                RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
                //TODO(Ray):ASSERT AFTER EVERY SET DEPTHSTENCILSTATE TO ENSURE WE GOT A VALID STATE BACKK                            
                if(!last_set_pass_desc.stencil_attachment.description.texture.state)
                {
                    //                                Assert(current_pass_desc.stencil_attachment.description.texture.state);
                    //Assert(prev_pass_desc.stencil_attachment.description.texture.state);
                    //NOTE(Ray):Since we need a valid texture set for the renderpass end encoding and
                    //start a new encoder with a valid texture set on the render pass descriptor.
//                    RendererCode::SetRenderPassDescriptor(&current_pass_desc);
                    RenderEncoderCode::EndEncoding(&in_params.re);
#if METALIZER_DEBUG_OUTPUT
                    PlatformOutput(s->debug_out_general,"End Encoding Setting renderpassdesc to have texture \n");
#endif
                    init_params = false;                                                          
                }
                s->is_stencil_enabled = true;
                continue;
            }
                
            else if(command_type == glemu_bufferstate_stencil_disable)
            {
                GLEMUStencilStateCommand* command = OGLE_POP(at,GLEMUStencilStateCommand);
                current_depth_desc.frontFaceStencil.enabled = false;
                current_depth_desc.backFaceStencil.enabled = false;
                DepthStencilState state = ogle_get_depth_sten_state(s,current_depth_desc);
                RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
                RenderDebug::PopDebugGroup(in_params.re);
#ifdef METALIZER_INSERT_DEBUGSIGNPOST
                char* string = "Stencil Disabled:";
                RenderDebug::InsertDebugSignPost(in_params.re,string);
#endif
                s->is_stencil_enabled = false;
                continue;
            }
                
            else if(command_type == glemu_bufferstate_stencil_mask)
            {
                GLEMUStencilMaskCommand* command = OGLE_POP(at,GLEMUStencilMaskCommand);
                current_depth_desc.frontFaceStencil.write_mask = command->write_mask_value;
                current_depth_desc.backFaceStencil.write_mask = command->write_mask_value;
                DepthStencilState state = ogle_get_depth_sten_state(s,current_depth_desc);
                RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
#if METALIZER_DEBUG_OUTPUT                                
                PlatformOutput(s->debug_out_general,"Framebuffer_stencil mask\n");
#endif
                continue;                            
            }
                
            else if(command_type == glemu_bufferstate_stencil_mask_sep)
            {
                GLEMUStencilMaskSepCommand* command = OGLE_POP(at,GLEMUStencilMaskSepCommand);
                if(command->front_or_back)
                {
                    current_depth_desc.frontFaceStencil.write_mask = command->write_mask_value;
                }
                else
                {
                    current_depth_desc.backFaceStencil.write_mask = command->write_mask_value;                                
                }
                DepthStencilState state = ogle_get_depth_sten_state(s,current_depth_desc);
                RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);

#if METALIZER_DEBUG_OUTPUT                                
                PlatformOutput(s->debug_out_general,"Framebuffer_stencil mask sep\n");
#endif
                continue;                            
            }

            else if(command_type == glemu_bufferstate_depth_func)
            {
                GLEMUDepthFuncCommand* command = OGLE_POP(at,GLEMUDepthFuncCommand);
                current_depth_desc.depthCompareFunction = command->func;
                DepthStencilState state = ogle_get_depth_sten_state(s,current_depth_desc);
                RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
#if METALIZER_DEBUG_OUTPUT                                
                PlatformOutput(s->debug_out_general,"Framebuffer_stencil func\n");
#endif
                continue;
            }
                
            else if(command_type == glemu_bufferstate_stencil_func)
            {
                GLEMUStencilFunCommand* command = OGLE_POP(at,GLEMUStencilFunCommand);
                current_depth_desc.frontFaceStencil.stencilCompareFunction = command->compareFunction;
                current_depth_desc.frontFaceStencil.read_mask = command->write_mask_value;
                current_depth_desc.backFaceStencil.stencilCompareFunction = command->compareFunction;
                current_depth_desc.backFaceStencil.read_mask = command->write_mask_value;
                RenderEncoderCode::SetStencilReferenceValue(in_params.re,command->mask_value);
                DepthStencilState state = ogle_get_depth_sten_state(s,current_depth_desc);
                RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
#if METALIZER_DEBUG_OUTPUT                                
                PlatformOutput(s->debug_out_general,"Framebuffer_stencil func\n");
#endif
                continue;
            }
                
            else if(command_type == glemu_bufferstate_stencil_func_sep)
            {
                GLEMUStencilFunSepCommand* command = OGLE_POP(at,GLEMUStencilFunSepCommand);
                if(command->front_or_back)
                {
                    current_depth_desc.frontFaceStencil.stencilCompareFunction = command->compareFunction;
                    current_depth_desc.frontFaceStencil.read_mask = command->write_mask_value;
                }
                else
                {
                    current_depth_desc.backFaceStencil.stencilCompareFunction = command->compareFunction;
                    current_depth_desc.backFaceStencil.read_mask = command->write_mask_value;
                }
                RenderEncoderCode::SetStencilReferenceValue(in_params.re,command->mask_value);
                DepthStencilState state = ogle_get_depth_sten_state(s,current_depth_desc);
                RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
#if METALIZER_DEBUG_OUTPUT                                
                PlatformOutput(s->debug_out_general,"Framebuffer_stencil func sep\n");
#endif
                continue;
            }
                
            else if(command_type == glemu_bufferstate_stencil_op)
            {
                GLEMUStencilOpCommand* command = OGLE_POP(at,GLEMUStencilOpCommand);
                current_depth_desc.frontFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                current_depth_desc.frontFaceStencil.depthFailureOperation = command->depth_fail_op;
                current_depth_desc.frontFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;
                    
                current_depth_desc.backFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                current_depth_desc.backFaceStencil.depthFailureOperation = command->depth_fail_op;
                current_depth_desc.backFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;

                if(s->is_stencil_enabled)
                {
                    DepthStencilState state = ogle_get_depth_sten_state(s,current_depth_desc);
#if METALIZER_DEBUG_OUTPUT                                
                    PlatformOutput(s->debug_out_general,"Framebuffer_stencil  op\n");
#endif
                    RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);                        
                }

                continue;
            }
                
            else if(command_type == glemu_bufferstate_stencil_op_sep)
            {
                GLEMUStencilOpSepCommand* command = OGLE_POP(at,GLEMUStencilOpSepCommand);
                if(command->front_or_back)
                {
                    current_depth_desc.frontFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                    current_depth_desc.frontFaceStencil.depthFailureOperation = command->depth_fail_op;
                    current_depth_desc.frontFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;
                }
                else
                {
                    current_depth_desc.backFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                    current_depth_desc.backFaceStencil.depthFailureOperation = command->depth_fail_op;
                    current_depth_desc.backFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;                                
                }
                DepthStencilState state = ogle_get_depth_sten_state(s,current_depth_desc);
                RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
#if METALIZER_DEBUG_OUTPUT                                
                PlatformOutput(s->debug_out_general,"Framebuffer_stencil op sep\n");
#endif
                continue;
            }
                
            else if(command_type == glemu_bufferstate_stencil_func_and_op)
            {
                GLEMUStencilFuncAndOpCommand* command = OGLE_POP(at,GLEMUStencilFuncAndOpCommand);
                current_depth_desc.frontFaceStencil.stencilCompareFunction = command->compareFunction;
                current_depth_desc.frontFaceStencil.read_mask = command->write_mask_value;
                current_depth_desc.backFaceStencil.stencilCompareFunction = command->compareFunction;
                current_depth_desc.backFaceStencil.read_mask = command->write_mask_value;
                RenderEncoderCode::SetStencilReferenceValue(in_params.re,command->mask_value);
                current_depth_desc.frontFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                current_depth_desc.frontFaceStencil.depthFailureOperation = command->depth_fail_op;
                current_depth_desc.frontFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;
                    
                current_depth_desc.backFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                current_depth_desc.backFaceStencil.depthFailureOperation = command->depth_fail_op;
                current_depth_desc.backFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;
                    
                DepthStencilState state = ogle_get_depth_sten_state(s,current_depth_desc);
                RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);

#if METALIZER_DEBUG_OUTPUT                                
                PlatformOutput(s->debug_out_general,"Framebuffer_stencil func and op\n");
#endif
                continue;
            }
                
            else if(command_type == glemu_bufferstate_stencil_func_and_op_sep)
            {
                GLEMUStencilFuncAndOpSepCommand* command = OGLE_POP(at,GLEMUStencilFuncAndOpSepCommand);
                if(command->front_or_back)
                {
                    current_depth_desc.frontFaceStencil.stencilCompareFunction = command->compareFunction;
                    current_depth_desc.frontFaceStencil.read_mask = command->write_mask_value;
                    current_depth_desc.frontFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                    current_depth_desc.frontFaceStencil.depthFailureOperation = command->depth_fail_op;
                    current_depth_desc.frontFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;
                }
                else
                {
                    current_depth_desc.backFaceStencil.stencilFailureOperation = command->stencil_fail_op;
                    current_depth_desc.backFaceStencil.depthFailureOperation = command->depth_fail_op;
                    current_depth_desc.backFaceStencil.depthStencilPassOperation = command->depth_stencil_pass_op;                                
                    current_depth_desc.backFaceStencil.stencilCompareFunction = command->compareFunction;
                    current_depth_desc.backFaceStencil.read_mask = command->write_mask_value;
                }
                RenderEncoderCode::SetStencilReferenceValue(in_params.re,command->mask_value);
                DepthStencilState state = ogle_get_depth_sten_state(s,current_depth_desc);
                RenderEncoderCode::SetDepthStencilState(&in_params.re,&state);
#if METALIZER_DEBUG_OUTPUT                                
                PlatformOutput(s->debug_out_general,"Framebuffer_stencil func and op sep\n");
#endif
                continue;
            }
                
            else if(command_type == glemu_bufferstate_draw_arrays || command_type == glemu_bufferstate_draw_elements)
            {
                GLEMUDrawArraysCommand* command;
                command = OGLE_POP(at,GLEMUDrawArraysCommand);                    

                //None means a draw attempt here we check bindings if there are any and set up the data
                //binding for the gpu along with sending any data to the gpu that needs sent.
                //TODO(Ray):Later once this is working switch form buffer 3 to 2 and remove the three buffer
                //and other uniforms setting in the uniform matrix excute callback.
                //TODO(Ray):make sure we can actually use the same buffer index for the gpu set bytes here.
                //TODO(Ray):Verifiy that draw index and for loop index i actually
                UniformBindingTableEntry uni_entry = ogle_get_uniform_entry_for_draw(s,command->uniform_table_index);
                if(uni_entry.v_size > 0)
                {
                    RenderEncoderCode::SetVertexBytes(&in_params.re,uni_entry.v_data,uni_entry.v_size,uni_entry.v_index);
#if METALIZER_INSERT_DEBUGSIGNPOST
                    PlatformOutput(debug_out_uniforms,"UniformBinding table entry : v_size :%d :  buffer index %d \n",uni_entry.v_size,4);
#endif
                }
                if(uni_entry.f_size > 0)
                {
                    RenderEncoderCode::SetFragmentBytes(&in_params.re,uni_entry.f_data,uni_entry.f_size,uni_entry.f_index);
#if METALIZER_INSERT_DEBUGSIGNPOST
                    PlatformOutput(debug_out_uniforms,"UnidformBinding table entry : f_size :%d : buffer index %d \n",uni_entry.f_size,uni_entry.f_index);
#endif
                }
                    
                //TExture bindings here
                //for ever texture bind at texture index
                //TODO(Ray):Allow for texture bindings on the vertex shader and compute functions.
                float2 t_buffer_range = command->texture_buffer_range;
                for(int i = t_buffer_range.x();i < t_buffer_range.y();++i)
                {
                    FragmentShaderTextureBindingTableEntry* entry = YoyoGetVectorElement(FragmentShaderTextureBindingTableEntry,&s->currently_bound_frag_textures,i);
                    Assert(entry);
                    Assert(entry->texture.texture.state);
                    Assert(entry->texture.sampler.state);
                    GLTexture final_tex = entry->texture;

                    if(ogle_is_valid_texture(s,final_tex))
                    {
#ifdef METALIZER_INSERT_DEBUGSIGNPOST
                        PlatformOutput(s->debug_out_high,"Attempt To bind invalid texture Invalid texture  \n");
#endif
                    }
                    else
                    {
                        final_tex = s->default_texture;
                    }
                        
                    Assert(!final_tex.texture.is_released);

//TODO(Ray):Change this define to GLEMUDIAG
#if YOYODIAG                        
                    for(int i = 0;i < resource_managment_tables.released_textures_table.anythings.count;++i)
                    {
                        ReleasedTextureEntry* rte = YoyoGetVectorElement(ReleasedTextureEntry,&resource_managment_tables.released_textures_table.anythings,i);
                        if(rte && !rte->is_free)
                        {
                            if(AnythingCacheCode::DoesThingExist(&gl_texturecache,&rte->tex_key))
                            {
                                GLTexture* tex = GetThingPtr(&gl_texturecache,&rte->tex_key,GLTexture);
                                Assert(tex->id == rte->tex_key.gl_tex_id);
                                Assert(tex->texture.state);
                                Assert(!tex->texture.is_released);
                                Assert(!tex->is_released);
                                if(tex->texture.state == final_tex.texture.state)
                                {
                                    PlatformOutput(true,"CANNOT USE TEXTURE AFTER RELEASE IS CALLED!! using tex_id %d released tex id %d \n",final_tex.id,tex->id);
                                    Assert(false);
                                }
                            }
                        }
                    }
#endif

                    RenderEncoderCode::SetFragmentTexture(&in_params.re,&final_tex.texture,entry->tex_index);
                    //TODO(Ray):Allow for mutliple sampler bindings or perhaps none and use shader defined ones
                    //for now all textures use the sampler index 0 and they must have one defined.
                    //In other words a GLTexture at teh moment means you have a sampler with you by default.
                    RenderEncoderCode::SetFragmentSamplerState(&in_params.re,&final_tex.sampler,entry->sampler_index);
#ifdef METALIZER_DEBUG_OUTPUT
                    PlatformOutput(s->debug_out_high,"texture binding entry : index:%d : range index %d \n",entry->tex_index,i);
                    PlatformOutput(s->debug_out_high,"sampler binding entry : index:%d : range index %d \n",entry->sampler_index,i);
#endif
                }
                    
                //TODO(Ray):For every vertex or fragment texture binding add a binding if there is no
                //binding for the shader at that index we should know right away we would need some introspection into the shader/
                //at that point but we dont have that yet for current projects its not an issue.
                    
                //TODO(Ray):If we ever need to GET shader meta data and check bindings match shader inputs.
                    
                //Buffer bindings
                //TODO(Ray):Allow for buffer bindings on the fragment and compute function
                float2 buffer_range = command->buffer_range;
                for(int i = buffer_range.x();i < buffer_range.y();++i)
                {
                    BufferBindingTableEntry* entry = YoyoGetVectorElement(BufferBindingTableEntry,&s->currently_bound_buffers,i);
                    GPUBuffer* buffer = ogle_get_gpu_buffer_binding(s,entry->key);
                    RenderEncoderCode::SetVertexBuffer(&in_params.re,buffer,entry->offset,entry->index);
                    // PlatformOutput(true,"buffer binding entry : index:%d : offset %d : range index %d \n",entry->index,entry->offset,i);
                }

                //TODO(Ray):We should split these out to get rid off the branching 
                RenderCommandEncoder re = in_params.re;
                uint32_t bi = s->current_buffer_index;
                uint32_t current_count = command->current_count;
                if(command_type == glemu_bufferstate_draw_elements)
                {
                    GPUBuffer* ebuffer = ogle_get_gpu_buffer_binding(s,command->element_buffer_id);
                    RenderEncoderCode::DrawIndexedPrimitives(&re,ebuffer, command->topology,command->current_count,command->index_type,command->offset);
                }
                else
                {
                    RenderEncoderCode::DrawPrimitives(&re, command->topology, command->offset, (current_count));                    
                }

            }
            else
            {
                Assert(false);
            }
        }
            
        RenderEncoderCode::AddCompletedHandler(c_buffer,[](void* arg)
                                                        {
                                                            DispatchSemaphoreT* sema = (DispatchSemaphoreT*)arg;
                                                            RenderSynchronization::DispatchSemaphoreSignal(sema);
                                                        },&s->semaphore);
            
        if(init_params)
        {
            RenderEncoderCode::EndEncoding(&in_params.re);            
        }

//#ifdef GLEMU_PRESENT           
        //Tell the gpu to present the drawable that we wrote to
        if(present)
        {
            RenderEncoderCode::PresentDrawable(c_buffer,current_drawable.state);            
        }

//#endif
        if(commit_cb)
        {
            RenderEncoderCode::Commit(c_buffer);            
        }
        else if(enqueue_cb)
        {
            RenderEncoderCode::Enqueue(c_buffer);
        }
        else
        {
//Not commiting or enqueuing the command buffer for execution assumes someone later down the line will
        }
            
        s->prev_frame_pipeline_state = in_params.pipeline_state;
    }
        
    //Render stats
    uint32_t pipelinestate_count = RenderCache::GetPipelineStateCount();
    PlatformOutput(true, "Renderpipeline states: %d\n",pipelinestate_count);
    uint32_t depth_stencil_state_count = RenderCache::GetPipelineStateCount();
    PlatformOutput(true, "DepthStencilState count: %d\n",depth_stencil_state_count);
    PlatformOutput(true, "RenderEncoder count: %d\n",render_encoder_count);
    PlatformOutput(true, "Draw count: %d\n",s->draw_index);
}

void ogle_execute_passthrough(OpenGLEmuState*s,void* pass_in_c_buffer)
{
    ogle_execute(s,pass_in_c_buffer,false,false);
}

void ogle_execute_commit(OpenGLEmuState*s,void* pass_in_c_buffer,bool present = false)
{
    ogle_execute(s,pass_in_c_buffer,false,true,present);
}

void ogle_execute_enqueue(OpenGLEmuState*s,void* pass_in_c_buffer)
{
    ogle_execute(s,pass_in_c_buffer,true,false);
}

#endif
