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
		RGBA32_SNORM,
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
		// TODO ��
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



	struct DescriptorSetBindingParams // DescriptorSet �Ɠ��l
	{
		enum DescriptorType {
			UniformBuffer_bit = 0x00000001,
			Sampler_bit = 0x00000010,
			Texture_bit = 0x00000100,
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

	struct DescriptorSetLayoutParams // DescriptorSetLayout �Ɠ��l
	{
		ValueArray<DescriptorSetBindingParams*> descriptorSetBindingParams;
		bool isBindless = false;
	};

	struct VertexAttributeLayout // VertexInputState �Ɠ��l
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

		// �Ƃ肠���� binding = 0 �Œ�
		ValueArray<Attributes> attributes;
		int stride = 0;
		int binding = 0;
	};

	struct ShaderStageParams // ShaderStage �Ɠ��l
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
	};

	struct RenderPassParams {
		std::string name;
		std::vector<AttachmentParams> attachments;
		std::vector<SubpassParams> subpasses;
		std::vector<SubpassDependencyParams> dependencies;

		bool isClearRenderPass = false;
		std::string clearRenderPassName = "";
	};

	struct GraphicsPipelineParams // GraphicsPipeline �Ɠ��l
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

	// TODO : vulkan �ˑ����Ȃ���
	template <class ValueType>
	static void SetVertexAttributeDescription2(VertexAttributeLayout* pVertexAttributeLayout)
	{
		// �G���[
		pVertexAttributeLayout->attributes[0].binding = 0; // binding �� vkCmdBindVertexBuffers �̎w��
		pVertexAttributeLayout->attributes[0].location = 0;
		pVertexAttributeLayout->attributes[0].format = typeConverterFormat<decltype(ValueType::position)>(); // glsl �ł� vec3
		pVertexAttributeLayout->attributes[0].offset = offsetof(ValueType, ValueType::position);

		pVertexAttributeLayout->attributes[1].binding = 0; // binding �� vkCmdBindVertexBuffers �̎w��
		pVertexAttributeLayout->attributes[1].location = 1;
		pVertexAttributeLayout->attributes[1].format = typeConverterFormat<decltype(ValueType::normal)>(); // glsl �ł� vec3
		pVertexAttributeLayout->attributes[1].offset = offsetof(ValueType, ValueType::normal);

		pVertexAttributeLayout->attributes[2].binding = 0; // binding �� vkCmdBindVertexBuffers �̎w��
		pVertexAttributeLayout->attributes[2].location = 2;
		pVertexAttributeLayout->attributes[2].format = typeConverterFormat<decltype(ValueType::color)>(); // glsl �ł� vec4
		pVertexAttributeLayout->attributes[2].offset = offsetof(ValueType, ValueType::color);

		pVertexAttributeLayout->attributes[3].binding = 0; // binding �� vkCmdBindVertexBuffers �̎w��
		pVertexAttributeLayout->attributes[3].location = 3;
		pVertexAttributeLayout->attributes[3].format = typeConverterFormat<decltype(ValueType::uv)>();
		pVertexAttributeLayout->attributes[3].offset = offsetof(ValueType, ValueType::uv);
	}

	template <class ValueType>
	static void CreateVertexAttributeLayout2(VertexAttributeLayout* pVertexAttributeLayout)
	{
		pVertexAttributeLayout->name = typeid(ValueType).name();
		pVertexAttributeLayout->binding = 0; // binding �� vkCmdBindVertexBuffers �̎w��
		pVertexAttributeLayout->stride = sizeof(ValueType);

		pVertexAttributeLayout->attributes.resize(4);
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
		// drawArray.cpp �Ɏ������Ȃ��Ǝ���
		pDrawArray->gpuInitialize(m_pImpl);
	}

	template <class ValueType>
	void RegisterVertexInputStateImpl();

	template <class ValueType>
	void UpdateVertexArray(DrawVertexArray<ValueType>* pDrawArray)
	{
		// drawArray.cpp �Ɏ������Ȃ��Ǝ���
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
