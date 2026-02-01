#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "util.h"

#define WIDTH 800
#define HEIGHT 600
#define MAX_FRAMES_IN_FLIGHT 2

typedef struct
{
  float x, y, r, g, b;
}Vertex;

static const Vertex vertices[4] = {
  {-0.5f, -0.5f, 1.0f, 0.0f, 0.0f},
  {0.5f, -0.5f, 0.0f, 1.0f, 0.0f},
  {0.5f, 0.5f, 0.0f, 0.0f, 1.0f},
  {-0.5f, 0.5f, 1.0f, 1.0f, 1.0f},
};

static const uint16_t indices[6] = {
  0, 1, 2, 2, 3, 0,
};

#define QUEUE_COUNT 2
typedef struct {
  long graphics_index;
  long presentation_index;
}QueueFamilyIndices;

QueueFamilyIndices queue_indices;

#define VALIDATION_LAYER_COUNT 1
const char *validation_layers[VALIDATION_LAYER_COUNT] = {"VK_LAYER_KHRONOS_validation"};

#define DEVICE_EXTENSION_COUNT 1
const char *device_extensions[DEVICE_EXTENSION_COUNT] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

GLFWwindow* window;
VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice logical_device;
VkQueue graphics_queue;
VkQueue presentation_queue;
VkSurfaceFormatKHR format;
VkPresentModeKHR present_mode;
VkSwapchainKHR swap_chain;
#define MAX_SWAP_CHAIN_IMGS 10 // TODO: Is this reasonable?
unsigned int swap_chain_img_count;
VkImage swap_chain_imgs[MAX_SWAP_CHAIN_IMGS];
VkFormat swap_chain_img_format;
VkExtent2D swap_chain_extent;
VkImageView swap_chain_img_views[MAX_SWAP_CHAIN_IMGS];
VkFramebuffer swap_chain_framebuffers[MAX_SWAP_CHAIN_IMGS];
VkRenderPass render_pass;
VkPipelineLayout pipeline_layout;
VkPipeline graphics_pipeline;
VkCommandPool command_pool;
VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];
VkSemaphore img_available_semaphores[MAX_FRAMES_IN_FLIGHT];
VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
VkBuffer vertex_buffer;
VkDeviceMemory vertex_buffer_mem;
VkBuffer index_buffer;
VkDeviceMemory index_buffer_mem;
bool framebuffer_resized = false;

static bool check_for_validation_layers();
static void create_instance();
static void create_surface();
static void select_physical_device();
static void create_logical_device();
static void create_swap_chain();
static void create_img_views();
static void create_render_pass();
static void create_graphics_pipeline();
static void create_framebuffers();
static void create_command_buffers();
static void create_command_pool();
static void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags mem_flags, VkBuffer *buffer, VkDeviceMemory *buffer_mem);
static void create_vertex_buffer();
static void create_index_buffer();
static void create_sync_prims();
static void recreate_swap_chain();
static void cleanup_swap_chain();
static void handle_framebuffer_resize(GLFWwindow*, int, int);

void init_window()
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, handle_framebuffer_resize);

}

void init_vulkan()
{
  create_instance();
  create_surface();
  select_physical_device();
  create_logical_device();
  create_swap_chain();
  create_img_views();
  create_render_pass();
  create_graphics_pipeline();
  create_framebuffers();
  create_command_pool();
  create_vertex_buffer();
  create_index_buffer();
  create_command_buffers();
  create_sync_prims();
}

void create_instance()
{
#ifdef DEBUG
  if (!check_for_validation_layers()) {
    fprintf(stderr, "ERROR: Missing validation layers\n");
    exit(1);
  }
#endif
  
  VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "VK TEMPLATE",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_0,
  };

  uint32_t glfw_extension_count = 0;
  const char** glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  VkInstanceCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app_info,
    .enabledExtensionCount = glfw_extension_count,
    .ppEnabledExtensionNames = glfw_extensions,
#ifdef DEBUG
    .enabledLayerCount = VALIDATION_LAYER_COUNT,
    .ppEnabledLayerNames = validation_layers,
#else
    .enabledLayerCount = 0,
#endif
  };
  
  VkResult result;
  if ((result = vkCreateInstance(&create_info, NULL, &instance)) != VK_SUCCESS) {
    fprintf(stderr, "ERROR: Could not create Vulkan instance\n");
    exit(1);
  }
}

void create_surface()
{
  if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
    fprintf(stderr, "ERROR: Failed to create window surface\n");
    exit(1);
  }
}

void select_physical_device()
{
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, NULL);

  if (device_count == 0) {
    fprintf(stderr, "ERROR: Cannot find GPU\n");
    exit(1);
  }

  VkPhysicalDevice devices[device_count];
  vkEnumeratePhysicalDevices(instance, &device_count, devices);

  //TODO: add options to select a different GPU, I only have one so this doesnt matter.
  physical_device = devices[0];
}

void find_queue_indices(VkPhysicalDevice device) {
  queue_indices = (QueueFamilyIndices) {.graphics_index = -1,
					.presentation_index = -1};
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

  VkQueueFamilyProperties queue_families[queue_family_count];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

  for (uint32_t i = 0; i < queue_family_count; ++i) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      queue_indices.graphics_index = i;
    }

    VkBool32 presentation_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &presentation_support);

    if (presentation_support) {
      queue_indices.presentation_index = i;
    }
  }
}

void create_logical_device()
{
  if (queue_indices.graphics_index < 0) {
    fprintf(stderr, "ERROR: Could not find graphics queue\n");
    exit(1);
  }

  if (queue_indices.presentation_index < 0) {
    fprintf(stderr, "ERROR: Could not find presentation queue\n");
    exit(1);
  }
  
  long indices[QUEUE_COUNT] = {queue_indices.graphics_index, queue_indices.presentation_index};
  VkDeviceQueueCreateInfo queue_create_infos[QUEUE_COUNT];

  float queue_priority = 1.0f;
  for (size_t i = 0; i < QUEUE_COUNT; ++i) {
    VkDeviceQueueCreateInfo queue_create_info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = indices[i],
      .queueCount = 1,
      .pQueuePriorities = &queue_priority,
    };
    queue_create_infos[i] = queue_create_info;
  }
  
  VkPhysicalDeviceFeatures device_features = {0};

  VkDeviceCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = 2,
    .pQueueCreateInfos = queue_create_infos,
    .pEnabledFeatures = &device_features,
#ifdef DEBUG
    .enabledLayerCount = VALIDATION_LAYER_COUNT,
    .ppEnabledLayerNames = validation_layers,
#else
    .enabledLayerCount = 0,
#endif
    .enabledExtensionCount = DEVICE_EXTENSION_COUNT,
    .ppEnabledExtensionNames = device_extensions,
  };

  if (vkCreateDevice(physical_device, &create_info, NULL, &logical_device) != VK_SUCCESS) {
    fprintf(stderr, "ERROR: Failed to create logical device!\n");
  }
  vkGetDeviceQueue(logical_device, queue_indices.graphics_index, 0, &graphics_queue);
  vkGetDeviceQueue(logical_device, queue_indices.presentation_index, 0, &presentation_queue);
}

VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR *capabilities)
{
  if (capabilities->currentExtent.width != UINT32_MAX) {
    return capabilities->currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actual_extent = {
      .width = (uint32_t) width,
      .height = (uint32_t) height
    };

    actual_extent.width = clamp_u32(actual_extent.width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
    actual_extent.height = clamp_u32(actual_extent.height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);

    return actual_extent;
  }  
}

VkSurfaceFormatKHR choose_swap_format(VkSurfaceFormatKHR *formats, uint32_t format_count)
{
  for (uint32_t i = 0; i < format_count; ++i) {
    if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return formats[i];
    }
  }

  return formats[0];
}

VkPresentModeKHR choose_swap_present_mode(VkPresentModeKHR *present_modes, uint32_t present_modes_count) {
  for (uint32_t i = 0; i < present_modes_count; ++i) {
    if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return present_modes[i];
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

void create_swap_chain()
{
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, NULL);
  assert(format_count > 0);
  VkSurfaceFormatKHR formats[format_count];
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats);

  uint32_t present_modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_modes_count, NULL);
  assert(present_modes_count > 0);
  VkPresentModeKHR present_modes[present_modes_count];
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_modes_count, present_modes);

  VkExtent2D extent = choose_swap_extent(&capabilities);
  format = choose_swap_format(formats, format_count);
  present_mode = choose_swap_present_mode(present_modes, present_modes_count);

  // TODO: make this higer?
  uint32_t img_count = capabilities.minImageCount + 1;
  uint32_t queue_family_indices[] = {queue_indices.graphics_index, queue_indices.presentation_index};
  VkSwapchainCreateInfoKHR create_info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = surface,
    .minImageCount = img_count,
    .imageFormat = format.format,
    .imageColorSpace = format.colorSpace,
    .imageExtent = extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform = capabilities.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = present_mode,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE,
  };
  
  if (queue_indices.graphics_index != queue_indices.presentation_index) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  if (vkCreateSwapchainKHR(logical_device, &create_info, NULL, &swap_chain) != VK_SUCCESS) {
    fprintf(stderr, "ERROR: Failed to create swap chain\n");
    exit(1);
  }

  vkGetSwapchainImagesKHR(logical_device, swap_chain, &img_count, NULL);
  assert(img_count < MAX_SWAP_CHAIN_IMGS);
  vkGetSwapchainImagesKHR(logical_device, swap_chain, &img_count, &swap_chain_imgs[0]);

  swap_chain_img_format = format.format;
  swap_chain_extent = extent;

  swap_chain_img_count = img_count;
}

void create_img_views()
{
  for (size_t i = 0; i < swap_chain_img_count; ++i) {
    VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = swap_chain_imgs[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = swap_chain_img_format,
      .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1,
    };
    
    if (vkCreateImageView(logical_device, &create_info, NULL, &swap_chain_img_views[i]) != VK_SUCCESS) {
      fprintf(stderr, "ERROR: Could not create image view %ld\n", i);
      exit(1);
    }
  }
}

void create_render_pass()
{
  VkAttachmentDescription color_attachment = {
    .format = swap_chain_img_format,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkAttachmentReference color_attachment_ref = {
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };
  
  VkSubpassDescription subpass = {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &color_attachment_ref,
  };

  VkSubpassDependency dependency = {
    .srcSubpass = VK_SUBPASS_EXTERNAL,
    .dstSubpass = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = 0,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };
  
  VkRenderPassCreateInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &color_attachment,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &dependency,
  };
  
  if (vkCreateRenderPass(logical_device, &render_pass_info, NULL, &render_pass) != VK_SUCCESS) {
    fprintf(stderr, "ERROR: Could not create render pass\n");
    exit(1);
  }
}

void create_graphics_pipeline()
{
  FileInfo vert_source = get_file_info("./shaders/vert.spv");
  FileInfo frag_source = get_file_info("./shaders/frag.spv");

  VkShaderModuleCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = vert_source.size,
    .pCode = (const uint32_t*) vert_source.content,
  };
  
  VkShaderModule vert_module;
  if (vkCreateShaderModule(logical_device, &create_info, NULL, &vert_module) != VK_SUCCESS) {
    fprintf(stderr, "ERROR: Could not create vertex shader module\n");
    exit(1);
  }

  create_info.codeSize = frag_source.size;
  create_info.pCode = (const uint32_t*) frag_source.content;

  VkShaderModule frag_module;
  if (vkCreateShaderModule(logical_device, &create_info, NULL, &frag_module) != VK_SUCCESS) {
    fprintf(stderr, "ERROR: Could not create fragment shader module\n");
    exit(1);
  }

  VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vert_module,
    .pName = "main",
  };

  VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = frag_module,
    .pName = "main",
  };

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

  VkVertexInputBindingDescription binding_desc = {
    .binding = 0,
    .stride = sizeof(Vertex),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  VkVertexInputAttributeDescription attrib_desc[2] = {0};
  attrib_desc[0].binding = 0;
  attrib_desc[0].location = 0;
  attrib_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
  attrib_desc[0].offset = 0;
 
  attrib_desc[1].binding = 0;
  attrib_desc[1].location = 1;
  attrib_desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrib_desc[1].offset = sizeof(float) * 2;
 
  VkPipelineVertexInputStateCreateInfo vert_input_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &binding_desc,
    .vertexAttributeDescriptionCount = 2,
    .pVertexAttributeDescriptions = attrib_desc,
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  VkPipelineViewportStateCreateInfo viewport_state = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1,
  };

  VkPipelineRasterizationStateCreateInfo rasterizer = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .lineWidth = 1.0f,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
  };

  VkPipelineMultisampleStateCreateInfo multisampling = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable = VK_FALSE,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  VkPipelineColorBlendAttachmentState color_blend_attachment = {
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable = VK_FALSE,
  };

  VkPipelineColorBlendStateCreateInfo color_blend_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment,
    .blendConstants[0] = 0.0f,
    .blendConstants[1] = 0.0f,
    .blendConstants[2] = 0.0f,
    .blendConstants[3] = 0.0f,
  };

  VkDynamicState dynamic_states[2] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };
  
  VkPipelineDynamicStateCreateInfo dynamic_state = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = (uint32_t) 2,
    .pDynamicStates = &dynamic_states[0],
  };

  VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 0,
    .pushConstantRangeCount = 0,
  };

  if (vkCreatePipelineLayout(logical_device, &pipeline_layout_info, NULL, &pipeline_layout) != VK_SUCCESS) {
    fprintf(stderr, "ERROR: Could not create graphics pipeline layout\n");
    exit(1);
  }

  VkGraphicsPipelineCreateInfo pipeline_info = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = 2,
    .pStages = shader_stages,
    .pVertexInputState = &vert_input_info,
    .pInputAssemblyState = &input_assembly,
    .pViewportState = &viewport_state,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pColorBlendState = &color_blend_info,
    .pDynamicState = &dynamic_state,
    .layout = pipeline_layout,
    .renderPass = render_pass,
    .subpass = 0,
  };

  if (vkCreateGraphicsPipelines(logical_device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &graphics_pipeline) != VK_SUCCESS) {
    fprintf(stderr, "ERROR: Could not create graphics pipeline\n");
    exit(1);
  }

  vkDestroyShaderModule(logical_device, frag_module, NULL);
  vkDestroyShaderModule(logical_device, vert_module, NULL);
}

void create_framebuffers()
{
  for (size_t i = 0; i < swap_chain_img_count; ++i) {
    VkImageView attachments[] = {
      swap_chain_img_views[i]
    };

    VkFramebufferCreateInfo frame_buffer_info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = render_pass,
      .attachmentCount = 1,
      .pAttachments = attachments,
      .width = swap_chain_extent.width,
      .height = swap_chain_extent.height,
      .layers = 1,
    };

    if (vkCreateFramebuffer(logical_device, &frame_buffer_info, NULL, &swap_chain_framebuffers[i]) != VK_SUCCESS) {
      fprintf(stderr, "ERROR: Failed to create framebuffer\n");
      exit(1);
    }
  }
}

void create_command_pool()
{
  VkCommandPoolCreateInfo pool_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queue_indices.graphics_index,
  };
  
  if (vkCreateCommandPool(logical_device, &pool_info, NULL, &command_pool) != VK_SUCCESS) {
    fprintf(stderr, "ERROR: Failed to create command pool\n");
    exit(1);
  }
}

void create_command_buffers()
{
  VkCommandBufferAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = command_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
  };

  if (vkAllocateCommandBuffers(logical_device, &alloc_info, &command_buffers[0]) != VK_SUCCESS) {
    fprintf(stderr, "ERROR: Failed to allocate command buffers\n");
    exit(1);
  }
}

void create_sync_prims()
{
  VkSemaphoreCreateInfo semaphore_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    if (vkCreateSemaphore(logical_device, &semaphore_info, NULL, &img_available_semaphores[i]) != VK_SUCCESS ||
	vkCreateSemaphore(logical_device, &semaphore_info, NULL, &render_finished_semaphores[i]) != VK_SUCCESS ||
	vkCreateFence(logical_device, &fence_info, NULL, &in_flight_fences[i]) != VK_SUCCESS) {
      fprintf(stderr, "ERROR: Failed to create synchronization primitives\n");
      exit(1);

    }
  }
}

bool check_for_validation_layers()
{
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, NULL);

  VkLayerProperties available_layers[layer_count];
  vkEnumerateInstanceLayerProperties(&layer_count, &available_layers[0]);
  
  for (size_t i = 0; i < VALIDATION_LAYER_COUNT; ++i) {
    const char *layer_name = validation_layers[i];
    bool found = false;

    for (size_t j = 0; j < layer_count; ++j) {
      if (strcmp(layer_name, available_layers[j].layerName) == 0) {
	found = true;
	break;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

void record_command_buffer(VkCommandBuffer command_buffer, uint32_t index)
{
  VkCommandBufferBeginInfo beign_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };

  if (vkBeginCommandBuffer(command_buffer, &beign_info) != VK_SUCCESS) {
    fprintf(stderr, "WARNING: Failed to begin recording command buffer\n");
    return;
  }

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  VkRenderPassBeginInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = render_pass,
    .framebuffer = swap_chain_framebuffers[index],
    .renderArea.offset = (VkOffset2D) {.x = 0, .y = 0},
    .renderArea.extent = swap_chain_extent,
    .clearValueCount = 1,
    .pClearValues = &clearColor,
  };

  vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = (float) swap_chain_extent.width,
    .height = (float) swap_chain_extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);
  
  VkRect2D scissor = {
    .offset = (VkOffset2D) {.x = 0, .y = 0},
    .extent = swap_chain_extent,
  };
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);

  VkBuffer vertex_buffers[] = {vertex_buffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(command_buffer, (uint32_t) (sizeof(indices)/sizeof(uint16_t)), 1, 0, 0, 0);
  vkCmdEndRenderPass(command_buffer);

  if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
    fprintf(stderr, "WARNING: Failed to record command buffer\n");
  }
}

void draw_frame()
{
  static uint32_t current_frame = 0;
  vkWaitForFences(logical_device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

  uint32_t img_index;
  VkResult result = vkAcquireNextImageKHR(logical_device, swap_chain, UINT64_MAX, img_available_semaphores[current_frame], VK_NULL_HANDLE, &img_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreate_swap_chain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    fprintf(stderr, "WARNING: Failed to acquire swap chain image\n");
    return;
  }

  vkResetFences(logical_device, 1, &in_flight_fences[current_frame]);
  
  vkResetCommandBuffer(command_buffers[current_frame], 0);
  record_command_buffer(command_buffers[current_frame], img_index);

  VkSemaphore wait_semaphores[] = {img_available_semaphores[current_frame]};
  VkSemaphore signal_semaphores[] = {render_finished_semaphores[current_frame]};
  VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = wait_semaphores,
    .pWaitDstStageMask = wait_stages,
    .commandBufferCount = 1,
    .pCommandBuffers = &command_buffers[current_frame],
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = signal_semaphores,
  };

  if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS) {
    fprintf(stderr, "WARNING: Failed to submit draw command buffer\n");
  }

  VkSwapchainKHR swap_chains[] = {swap_chain};
  VkPresentInfoKHR present_info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = signal_semaphores,
    .swapchainCount = 1,
    .pSwapchains = swap_chains,
    .pImageIndices = &img_index,
  };
  
  result = vkQueuePresentKHR(presentation_queue, &present_info);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
    framebuffer_resized = false;
    recreate_swap_chain();
  } else if (result != VK_SUCCESS) {
    fprintf(stderr, "WARNING: Failed to present swap chain image\n");
    return;
  }

  current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags mem_flags, VkBuffer *buffer, VkDeviceMemory *buffer_mem)
{
  VkBufferCreateInfo buffer_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage_flags,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  if (vkCreateBuffer(logical_device, &buffer_info, NULL, buffer) != VK_SUCCESS) {
    fprintf(stderr, "ERROR: Failed to create buffer\n");
    exit(1);
  }

  VkMemoryRequirements mem_reqs;
  vkGetBufferMemoryRequirements(logical_device, *buffer, &mem_reqs);

  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  bool found = false;
  uint32_t mem_index;
  uint32_t type_filter = mem_reqs.memoryTypeBits;
  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & mem_flags) == mem_flags) {
      found = true;
      mem_index = i;
      break;
    }
  }
  
  if (!found) {
    fprintf(stderr, "ERROR: Failed to find suitable memory type\n");
    exit(1);
  }

  VkMemoryAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = mem_reqs.size,
    .memoryTypeIndex = mem_index,
  };

  if (vkAllocateMemory(logical_device, &alloc_info, NULL, buffer_mem) != VK_SUCCESS) {
    fprintf(stderr, "ERROR: Failed to allocate vertex buffer memory\n");
    exit(1);
  }
  
  vkBindBufferMemory(logical_device, *buffer, *buffer_mem, 0);
  
}
void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
  VkCommandBufferAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandPool = command_pool,
    .commandBufferCount = 1,
  };

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(logical_device, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  vkBeginCommandBuffer(command_buffer, &begin_info);

  VkBufferCopy copy_region = {
    .size = size,
  };
  vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &command_buffer,
  };
  
  vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphics_queue);

  vkFreeCommandBuffers(logical_device, command_pool, 1, &command_buffer);
}

void create_vertex_buffer()
{
  VkDeviceSize buffer_size = sizeof(vertices);

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_mem;
  create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_mem);

  void* data;
  vkMapMemory(logical_device, staging_buffer_mem, 0, (size_t) buffer_size, 0, &data);
  memcpy(data, vertices, (size_t) buffer_size);
  vkUnmapMemory(logical_device, staging_buffer_mem);

  create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertex_buffer, &vertex_buffer_mem);

  copy_buffer(staging_buffer, vertex_buffer, buffer_size);

  vkDestroyBuffer(logical_device, staging_buffer, NULL);
  vkFreeMemory(logical_device, staging_buffer_mem, NULL);
}

void create_index_buffer()
{
  VkDeviceSize buffer_size = sizeof(indices);

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_mem;
  create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_mem);

  void* data;
  vkMapMemory(logical_device, staging_buffer_mem, 0, buffer_size, 0, &data);
  memcpy(data, indices, (size_t) buffer_size);
  vkUnmapMemory(logical_device, staging_buffer_mem);

  create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &index_buffer, &index_buffer_mem);

  copy_buffer(staging_buffer, index_buffer, buffer_size);

  vkDestroyBuffer(logical_device, staging_buffer, NULL);
  vkFreeMemory(logical_device, staging_buffer_mem, NULL);
}

void main_loop()
{
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    draw_frame();
  }

  vkDeviceWaitIdle(logical_device);
}

void recreate_swap_chain()
{
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }
  
  vkDeviceWaitIdle(logical_device);
  cleanup_swap_chain();
  create_swap_chain();
  create_img_views();
  create_framebuffers();
}

void cleanup_swap_chain()
{
  for (size_t i = 0; i < swap_chain_img_count; ++i) {
    vkDestroyFramebuffer(logical_device, swap_chain_framebuffers[i], NULL);
  }
  for (size_t i = 0; i < swap_chain_img_count; ++i) {
    vkDestroyImageView(logical_device, swap_chain_img_views[i], NULL);
  }
  vkDestroySwapchainKHR(logical_device, swap_chain, NULL);
}

void cleanup()
{
  cleanup_swap_chain();
  vkDestroyBuffer(logical_device, index_buffer, NULL);
  vkFreeMemory(logical_device, index_buffer_mem, NULL);
  vkDestroyBuffer(logical_device, vertex_buffer, NULL);
  vkFreeMemory(logical_device, vertex_buffer_mem, NULL);
  vkDestroyPipeline(logical_device, graphics_pipeline, NULL);
  vkDestroyPipelineLayout(logical_device, pipeline_layout, NULL);
  vkDestroyRenderPass(logical_device, render_pass, NULL);
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(logical_device, img_available_semaphores[i], NULL);
    vkDestroySemaphore(logical_device, render_finished_semaphores[i], NULL);
    vkDestroyFence(logical_device, in_flight_fences[i], NULL);
  }
  vkDestroyCommandPool(logical_device, command_pool, NULL);
  vkDestroyDevice(logical_device, NULL);
  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyInstance(instance, NULL);
  glfwDestroyWindow(window);
  glfwTerminate();
}

void handle_framebuffer_resize(GLFWwindow*, int, int)
{
  framebuffer_resized = true;
}

int main()
{
  init_window();
  init_vulkan();
  main_loop();
  cleanup();
  return 0;
}
