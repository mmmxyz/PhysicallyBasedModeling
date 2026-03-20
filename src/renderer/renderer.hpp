#pragma once

#include <vector>
#include "src/utils/mathfunc/mathfunc.hpp"
#include "src/utils/memory/array.hpp"

class RendererImpl;

template <class ValueType>
class DrawVertexArray;

class GpuMemoryImpl;
class GpuTextureMemoryImpl;
class DescriptorSetImpl;

class Renderer {
private:
	RendererImpl* m_pImpl = nullptr;

public:
	enum ShaderStage {
		VertexBit = 0x00000001,
		FragmentBit = 0x00000010,
		ComputeBit = 0x00000011,
	};

	enum BufferCreateUsage {
		Uniform,
		Vertex,
		VertexIndex,
		Transfer
	};

	struct GpuBuffer {
		//uint32_t size;
		GpuMemoryImpl* pGpuMemoryImpl = nullptr;
	};

	struct GpuTexture {
		//uint32_t width;
		//uint32_t height;
		//uint32_t size;
		GpuTextureMemoryImpl* pGpuTextureMemoryImpl = nullptr;
	};

	struct DescriptorSetInterface {
		DescriptorSetImpl* pDescriptorSetImpl = nullptr;
	};

	struct DescriptorWriterParams {
		struct DescriptorInfo {
			enum DescriptorType {
				UniformBuffer,
				Sampler,
				Texture,
				Combined_Image_Sampler,
				InputAttachment,
			} type;
			uint32_t bindingNum;
			uint32_t count;
			ValueArray<void*> pResources; // GpuMemoryImpl* or GpuTextureMemoryImpl*
		};
		ValueArray<DescriptorInfo> descriptorInfos;
	};

	enum VertexAttributeFormat
	{
		Float,
		Int,
		Vec2,
		Vec3,
		Vec4,
		Mat2,
		Mat3,
		Mat4,
		Error
	};

	enum ShaderStageType {
		ShaderStageVertex,
		ShaderStageFragment,
	};

	enum ImageFormat {
		Undef,
		SameAsSwapChain,
		RGBA8_SNORM,
		RGBA8_UNORM,
		RGBA16_SFLOAT,
		RGBA32_FLOAT,
		R32G32B32A32_FLOAT,
		DEPTH16_UNORM,
		DEPTH32_SFLOAT,
	};


	enum ImageLayout {
		Undefined,
		General,
		ColorAttachmentOptimal,
		DepthStencilAttachmentOptimal,
		ShaderReadOnlyOptimal,
		TransferSrcOptimal,
		TransferDstOptimal,
		PreInitialized,
		DepthReadOnlyStencilAttachmentOptimal,
		PresentSrcKHR,
		// TODO 追加
	};

	enum AttatchmentLabel {
		UseSwapChainAttachment,
		ColorAttachment,
		DepthAttachment,
		NormalAttachment,
		PositionAttachment,
		AlbedoAttachment,
		MetallicRoughnessAttachment,
		Count,
	};

	enum ClearColorType {
		ClearColor,
		ClearDepthStancil
	};

	enum CompareOperator {
		Less,
		LessEqual,
		Greater,
		GreaterEqual,
		Equal,
		NotEqual,
		Always,
		Never,
	};

	enum class PipelineStageFlagBits : uint32_t {
		None = 0,
		TopOfPipe = 0x00000001,
		DrawIndirect = 0x00000002,
		VertexInput = 0x00000004,
		VertexShader = 0x00000008,
		FragmentShader = 0x00000080,
		EarlyFragmentTests = 0x00000100,
		LateFragmentTests = 0x00000200,
		ColorAttachmentOutput = 0x00000400,
		ComputeShader = 0x00000800,
		Transfer = 0x00001000,
		BottomOfPipe = 0x00002000,
		AllGraphics = 0x00008000,
		AllCommands = 0x00010000,
	};

	enum class AccessFlagBits : uint32_t {
		None = 0,
		IndirectCommandRead = 0x00000001,
		IndexRead = 0x00000002,
		VertexAttributeRead = 0x00000004,
		UniformRead = 0x00000008,
		InputAttachmentRead = 0x00000010,
		ShaderRead = 0x00000020,
		ShaderWrite = 0x00000040,
		ColorAttachmentRead = 0x00000080,
		ColorAttachmentWrite = 0x00000100,
		DepthStencilAttachmentRead = 0x00000200,
		DepthStencilAttachmentWrite = 0x00000400,
		TransferRead = 0x00000800,
		TransferWrite = 0x00001000,
		HostRead = 0x00002000,
		HostWrite = 0x00004000,
		MemoryRead = 0x00008000,
		MemoryWrite = 0x00010000,
	};

	enum class DependencyFlagBits : uint32_t {
		None = 0,
		ByRegion = 0x00000001,
		ViewLocal = 0x00000002,
		DeviceGroup = 0x00000004,
	};

	struct CreateImageParams
	{
		uint32_t width;
		uint32_t height;
		ImageFormat format;
		bool isColorAttatchment = false;
		bool isDepthStencilAttatchment = false;
		bool isInputAttatchment = false;
	};


	GpuBuffer CreateGpuBuffer(uint32_t size, BufferCreateUsage usage);
	GpuTexture CreateGpuTexture(CreateImageParams& createImageParams);
	GpuTexture GetRenderPassAttatchmentTexture(std::string renderPassName, Renderer::AttatchmentLabel label);
	DescriptorSetInterface CreateDescriptorSetInterface(std::string graphicsPipelineName, int set);
	DescriptorSetInterface CreateDescriptorSetInterface(std::string graphicsPipelineName, int set, bool isBindless, int bindlessCounter);

	void WriteDescriptorSet(DescriptorWriterParams& descriptorWriteParams, DescriptorSetInterface& descriptorSetInterface);

	void GetCpuMemoryPointer(GpuBuffer& gpuMemory, void** ppData);
	void UnmapCpuMemoryPointer(GpuBuffer& gpuMemoryImpl);
	void TransferStagingBufferToImage(GpuBuffer& stagingBuffer, GpuTexture& textureMemory);



	struct DescriptorSetBindingParams // DescriptorSet と同じ
	{
		enum DescriptorType {
			UniformBuffer_bit = 0x00000001,
			Sampler_bit = 0x00000010,
			Texture_bit = 0x00000100,
			InputAttachment_bit = 0x00001000,
		};

		DescriptorType type;

		enum ShaderStage : uint32_t {
			Vertex_bit = 0x00000001,
			Fragment_bit = 0x00000010,
		};

		uint32_t shaderStage;
		uint32_t bindingNum;
		uint32_t count;
	};

	struct DescriptorSetLayoutParams // DescriptorSetLayout と同じ
	{
		ValueArray<DescriptorSetBindingParams*> descriptorSetBindingParams;
		bool isBindless = false;
	};

	struct VertexAttributeLayout // VertexInputState と同じ
	{
		std::string name;
		struct Attributes {
		public:
			//std::string name = ""; // location
			int location = 0;
			int size = 0;
			VertexAttributeFormat format;
			int offset = 0;
			int binding = 0;
		};

		// とりあえず binding = 0 で固定
		ValueArray<Attributes> attributes;
		int stride = 0;
		int binding = 0;
	};

	struct ShaderStageParams // ShaderStage と同じ
	{
		std::string shaderPath = "";
		int specializationConstantCount;
		ShaderStageType stageType;
	};

	struct AttachmentParams {
		ImageFormat format = Undef;
		bool clear = true;
		bool store = true;
		ImageLayout initialLayout = Undefined;
		ImageLayout finalLayout = ColorAttachmentOptimal;

		AttatchmentLabel attachmentLabel = ColorAttachment;

		bool isColorAttatchment = false;
		bool isDepthStencilAttatchment = false;
		bool isInputAttatchment = false;
	};

	struct SubpassParams {
		std::vector<int> colorAttachments; // 特定パス内のindex
		std::vector<int> inputAttachments; // 特定パス内のindex
		int depthAttathment;
	};

	struct SubpassDependencyParams {
		int srcSubpass = -1; // 特定パス内のindex
		int dstSubpass = -1;
		PipelineStageFlagBits srcStageMask = PipelineStageFlagBits::None;
		PipelineStageFlagBits dstStageMask = PipelineStageFlagBits::None;
		AccessFlagBits srcAccessMask = AccessFlagBits::None;
		AccessFlagBits dstAccessMask = AccessFlagBits::None;
		DependencyFlagBits dependencyFlags = DependencyFlagBits::None;
	};

	struct RenderPassParams {
		std::string name;
		std::vector<AttachmentParams> attachments;
		std::vector<SubpassParams> subpasses;
		std::vector<SubpassDependencyParams> dependencies;

		bool isClearRenderPass = false;
		std::string clearRenderPassName = "";
	};

	struct GraphicsPipelineParams // GraphicsPipeline と同じ
	{
		std::string name;
		ValueArray<ShaderStageParams*> shaders;
		std::string vertexLayoutName;
		std::string renderPassName;
		int32_t subpassIndex = 0;
		ValueArray<DescriptorSetLayoutParams*> descriptorSetParams;
		CompareOperator depthOperator = CompareOperator::Greater;
		bool depthTestEnable = true;
		bool depthWriteEnable = true;
		int pushConstantSize;
		// TODO primitive, blend, rasterization, depthstencil state
	};

	template <typename AttributeType>
	static VertexAttributeFormat typeConverterFormat()
	{
		return VertexAttributeFormat::Error;
	}

		// TODO : vulkan に依存しない
	template <class ValueType>
	static void SetVertexAttributeDescription2(VertexAttributeLayout* pVertexAttributeLayout)
	{
		pVertexAttributeLayout->attributes[0].binding = 0;
		pVertexAttributeLayout->attributes[0].location = 0;
		pVertexAttributeLayout->attributes[0].format = typeConverterFormat<decltype(ValueType::position)>();
		pVertexAttributeLayout->attributes[0].offset = offsetof(ValueType, ValueType::position);

		pVertexAttributeLayout->attributes[1].binding = 0;
		pVertexAttributeLayout->attributes[1].location = 1;
		pVertexAttributeLayout->attributes[1].format = typeConverterFormat<decltype(ValueType::normal)>();
		pVertexAttributeLayout->attributes[1].offset = offsetof(ValueType, ValueType::normal);

		pVertexAttributeLayout->attributes[2].binding = 0;
		pVertexAttributeLayout->attributes[2].location = 2;
		pVertexAttributeLayout->attributes[2].format = typeConverterFormat<decltype(ValueType::color)>();
		pVertexAttributeLayout->attributes[2].offset = offsetof(ValueType, ValueType::color);

		pVertexAttributeLayout->attributes[3].binding = 0;
		pVertexAttributeLayout->attributes[3].location = 3;
		pVertexAttributeLayout->attributes[3].format = typeConverterFormat<decltype(ValueType::uv)>();
		pVertexAttributeLayout->attributes[3].offset = offsetof(ValueType, ValueType::uv);

		pVertexAttributeLayout->attributes[4].binding = 0;
		pVertexAttributeLayout->attributes[4].location = 4;
		pVertexAttributeLayout->attributes[4].format = typeConverterFormat<decltype(ValueType::tangent)>();
		pVertexAttributeLayout->attributes[4].offset = offsetof(ValueType, ValueType::tangent);

		pVertexAttributeLayout->attributes[5].binding = 0;
		pVertexAttributeLayout->attributes[5].location = 5;
		pVertexAttributeLayout->attributes[5].format = typeConverterFormat<decltype(ValueType::roughness)>();
		pVertexAttributeLayout->attributes[5].offset = offsetof(ValueType, ValueType::roughness);
	}

	template <class ValueType>
	static void CreateVertexAttributeLayout2(VertexAttributeLayout* pVertexAttributeLayout)
	{
		pVertexAttributeLayout->name = typeid(ValueType).name();
		pVertexAttributeLayout->binding = 0; // binding は vkCmdBindVertexBuffers の指定
		pVertexAttributeLayout->stride = sizeof(ValueType);

		pVertexAttributeLayout->attributes.resize(6);
		SetVertexAttributeDescription2<ValueType>(pVertexAttributeLayout);
	}

	void RegisterVertexInputStateImpl3(VertexAttributeLayout* vertexAttributeLayout);

	class InitializeParams {
	public:
		bool isDebugMode = false;
		ivec2 windowSize = ivec2(800, 600);
		std::string windowName;
	};

	void Initialize(InitializeParams& initializeParams);

	void CreateGraphicsPipeline(GraphicsPipelineParams& graphicsPipelineParams);

	void CreateRenderPass(RenderPassParams& renderPassParams);

	template <class ValueType>
	void InitializeVertexArray(DrawVertexArray<ValueType>* pDrawArray)
	{
		// drawArray.cpp に実装しないと駄目
		pDrawArray->gpuInitialize(m_pImpl);
	}

	template <class ValueType>
	void RegisterVertexInputStateImpl();

	template <class ValueType>
	void UpdateVertexArray(DrawVertexArray<ValueType>* pDrawArray)
	{
		// drawArray.cpp に実装しないと駄目
		pDrawArray->updateGpuMemory(m_pImpl);
	}

	class UpdatePushConstantParams
	{
	public:
		std::string graphicsPipelineName;
		int shaderStage;
		void* pData = nullptr;
		int32_t size = 0;
	};

	void UpdatePushConstant(UpdatePushConstantParams& pushConstantParams);

	union ClearColorValue {
		fvec4 color;
		struct {
			float depth;
			uint32_t stencil;
		} depthStencil;
		ClearColorValue() : color(fvec4::zero()) {}
	};

	class BeginRenderPassParams {
	public:
		std::string renderPassName;
		std::vector< Renderer::ClearColorType> clearColors;
		std::vector<ClearColorValue> clearColorValues;
	};

	class DrawParams {
	public:
		GpuMemoryImpl* pVertexArray;
		uint32_t count; // index or vertex 
		uint32_t instanceCount;
		GpuMemoryImpl* pIndexArray;
		std::vector<DescriptorSetInterface> descriptorSetInterfaces;
		std::string graphicsPipelineName;
	};

	class ClearRrenderPassAttatchmentParams {
		class ClearRenderPassAttatchmentInfo {
		public:
			enum ClearRrenderPassAttatchmentType {
				ClearColor,
				ClearDepthStancil
			} type;
			AttatchmentLabel label;
			int attachmentIdx;
			fvec4 clearColor;
			float clearDepth;
		};
	public:
		std::string renderPassName;
		std::vector<ClearRenderPassAttatchmentInfo> attatchmentInfos;
	};

	uint32_t counter = 0;
	uint32_t frameBufferIndex = 999;
	bool DrawCondition();
	void DrawStart();
	void BeginRenderPass(BeginRenderPassParams& beginRenderPassParams);
	void ClearRenderPassAttatchment(ClearRrenderPassAttatchmentParams& clearRenderPassAttatchmentParams);
	void Draw(DrawParams& drawParams);
	void NextSubpass();
	void EndRenderPass();
	void DrawEnd();
};

template <>
inline Renderer::VertexAttributeFormat Renderer::typeConverterFormat<fvec2>()
{
	return  Renderer::VertexAttributeFormat::Vec2;
}
template <>
inline  Renderer::VertexAttributeFormat Renderer::typeConverterFormat<fvec3>()
{
	return  Renderer::VertexAttributeFormat::Vec3;
}
template <>
inline  Renderer::VertexAttributeFormat Renderer::typeConverterFormat<fvec4>()
{
	return  Renderer::VertexAttributeFormat::Vec4;
}
template <>
inline  Renderer::VertexAttributeFormat Renderer::typeConverterFormat<float>()
{
	return  Renderer::VertexAttributeFormat::Float;
}

