#pragma once

#include <vector>
#include "src/utils/mathfunc/mathfunc.hpp"
#include "src/utils/geometry/array.hpp"

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
	enum BufferCreateUsage {
		Uniform,
		Vertex,
		Transfer
	};

	struct GpuBuffer {
		uint32_t size;
		GpuMemoryImpl* pGpuMemoryImpl = nullptr;
	};

	struct GpuTexture {
		uint32_t width;
		uint32_t height;
		uint32_t size;
		GpuTextureMemoryImpl* pGpuTextureMemoryImpl = nullptr;
	};

	struct DescriptorSetInterface {
		int set;
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
		ValueArray<DescriptorInfo*> descriptorInfos;
	};

	GpuBuffer CreateGpuBuffer(uint32_t size, BufferCreateUsage usage);
	GpuTexture CreateGpuTexture(uint32_t width, uint32_t height);
	DescriptorSetInterface CreateDescriptorSetInterface(std::string graphicsPipelineName, int set);
	void WriteDescriptorSet(DescriptorWriterParams& descriptorWriteParams, DescriptorSetInterface& descriptorSetInterface);

	void GetCpuMemoryPointer(GpuBuffer& gpuMemory, void** ppData);
	void UnmapCpuMemoryPointer(GpuBuffer& gpuMemoryImpl);
	void TransferStagingBufferToImage(GpuBuffer& stagingBuffer, GpuTexture& textureMemory);

	struct DescriptorSetBindingParams // DescriptorSet �Ɠ��l
	{
		enum DescriptorType {
			UniformBuffer_bit = 0x00000001,
			Sampler_bit	  = 0x00000010,
			Texture_bit	  = 0x00000100,
		};

		DescriptorType type;

		enum ShaderStage {
			Vertex_bit   = 0x00000001,
			Fragment_bit = 0x00000010,
		};

		ShaderStage shaderStage;
		uint32_t bindingNum;
		uint32_t count;
	};

	struct DescriptorSetLayoutParams // DescriptorSetLayout �Ɠ��l
	{
		ValueArray<DescriptorSetBindingParams*> descriptorSetBindingParams;
	};

	struct VertexAttributeLayout // VertexInputState �Ɠ��l
	{
		std::string name;
		struct Attributes {
		    public:
			//std::string name = ""; // location
			int location = 0;
			int size     = 0;
			enum AttributeFormat {
				Float,
				Int,
				Vec2,
				Vec3,
				Vec4,
				Mat2,
				Mat3,
				Mat4,
				Error
			} format;
			int offset  = 0;
			int binding = 0;
		};

		// �Ƃ肠���� binding = 0 �Œ�
		ValueArray<Attributes> attributes;
		int stride  = 0;
		int binding = 0;
	};

	struct ShaderStageParams // ShaderStage �Ɠ��l
	{
		std::string shaderPath = "";
		int specializationConstantCount;
		enum ShaderStageType {
			Vertex,
			Fragment,
		} stageType;
	};

	struct AttachmentParams {
		enum Format {
			Undef,
			SameAsSwapChain,
			RGBA8_SNORM,
			DEPTH16_UNORM,
		};

		Format format = Undef;
		bool clear    = true;
		bool store    = true;
		// initialLayout
		// finalLayout
	};

	struct SubpassParams {
		std::vector<int> colorAttachments;
		int depthAttathment;
	};

	struct RenderPassParams {
		std::string name;
		std::vector<AttachmentParams> attachments;
		std::vector<SubpassParams> subpasses;
	};

	struct GraphicsPipelineParams // GraphicsPipeline �Ɠ��l
	{
		std::string name;
		ValueArray<ShaderStageParams*> shaders;
		std::string vertexLayoutName;
		std::string renderPassName;
		ValueArray<DescriptorSetLayoutParams*> descriptorSetParams;
		int pushConstant;
		// TODO primitive, blend, rasterization, depthstencil state
	};

	template <typename AttributeType>
	static VertexAttributeLayout::Attributes::AttributeFormat typeConverterFormat()
	{
		return VertexAttributeLayout::Attributes::AttributeFormat::Error;
	}

	// TODO : vulkan �ˑ����Ȃ���
	template <class ValueType>
	static void SetVertexAttributeDescription2(VertexAttributeLayout* pVertexAttributeLayout)
	{
		// �G���[
		pVertexAttributeLayout->attributes[0].binding  = 0; // binding �� vkCmdBindVertexBuffers �̎w��
		pVertexAttributeLayout->attributes[0].location = 0;
		pVertexAttributeLayout->attributes[0].format   = typeConverterFormat<decltype(ValueType::position)>(); // glsl �ł� vec3
		pVertexAttributeLayout->attributes[0].offset   = offsetof(ValueType, ValueType::position);

		pVertexAttributeLayout->attributes[1].binding  = 0; // binding �� vkCmdBindVertexBuffers �̎w��
		pVertexAttributeLayout->attributes[1].location = 1;
		pVertexAttributeLayout->attributes[1].format   = typeConverterFormat<decltype(ValueType::normal)>(); // glsl �ł� vec3
		pVertexAttributeLayout->attributes[1].offset   = offsetof(ValueType, ValueType::normal);

		pVertexAttributeLayout->attributes[2].binding  = 0; // binding �� vkCmdBindVertexBuffers �̎w��
		pVertexAttributeLayout->attributes[2].location = 2;
		pVertexAttributeLayout->attributes[2].format   = typeConverterFormat<decltype(ValueType::color)>(); // glsl �ł� vec4
		pVertexAttributeLayout->attributes[2].offset   = offsetof(ValueType, ValueType::color);

		pVertexAttributeLayout->attributes[3].binding  = 0; // binding �� vkCmdBindVertexBuffers �̎w��
		pVertexAttributeLayout->attributes[3].location = 3;
		pVertexAttributeLayout->attributes[3].format   = typeConverterFormat<decltype(ValueType::uv)>();
		pVertexAttributeLayout->attributes[3].offset   = offsetof(ValueType, ValueType::uv);
	}

	template <class ValueType>
	static void CreateVertexAttributeLayout2(VertexAttributeLayout* pVertexAttributeLayout)
	{
		pVertexAttributeLayout->name	= typeid(ValueType).name();
		pVertexAttributeLayout->binding = 0; // binding �� vkCmdBindVertexBuffers �̎w��
		pVertexAttributeLayout->stride	= sizeof(ValueType);

		pVertexAttributeLayout->attributes.resize(4);
		SetVertexAttributeDescription2<ValueType>(pVertexAttributeLayout);
	}

	void RegisterVertexInputStateImpl3(VertexAttributeLayout* vertexAttributeLayout);

	class InitializeParams {
	    public:
		bool isDebugMode = false;
		ivec2 windowSize = ivec2(800, 600);
		std::string windowName;

		GraphicsPipelineParams* pGraphicsPipelineParams;
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

	class DrawParams {
	    public:
		std::vector<GpuMemoryImpl*> vertexArray;
		DescriptorSetInterface descriptorSetInterface;
		std::string graphicsPipelineName;
	};

	uint32_t counter	  = 0;
	uint32_t frameBufferIndex = 999;
	void Draw(DrawParams& drawParams);
	bool DrawCondition();
};

template <>
inline Renderer::VertexAttributeLayout::Attributes::AttributeFormat Renderer::typeConverterFormat<fvec2>()
{
	return VertexAttributeLayout::Attributes::AttributeFormat::Vec2;
}
template <>
inline Renderer::VertexAttributeLayout::Attributes::AttributeFormat Renderer::typeConverterFormat<fvec3>()
{
	return VertexAttributeLayout::Attributes::AttributeFormat::Vec3;
}
template <>
inline Renderer::VertexAttributeLayout::Attributes::AttributeFormat Renderer::typeConverterFormat<fvec4>()
{
	return VertexAttributeLayout::Attributes::AttributeFormat::Vec4;
}
