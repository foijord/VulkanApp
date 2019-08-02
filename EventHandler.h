#pragma once

#include <Innovator/RenderManager.h>
#include <Innovator/Nodes.h>

#include <QObject>
#include <QMouseEvent>
#include <QEvent>
#include <QApplication>
#include <QWindow>

#include <memory>

class QTextureImage : public VulkanTextureImage {
public:
  NO_COPY_OR_ASSIGNMENT(QTextureImage)

    explicit QTextureImage(const std::string & filename) :
    image(QImage(filename.c_str()))
  {
    this->image = this->image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
  }

  virtual ~QTextureImage() = default;

  VkExtent3D extent(size_t) const override
  {
    return {
      static_cast<uint32_t>(this->image.size().width()),
      static_cast<uint32_t>(this->image.size().height()),
      1
    };
  }

  uint32_t base_level() const override
  {
    return 0;
  }

  uint32_t levels() const override
  {
    return 1;
  }

  uint32_t base_layer() const override
  {
    return 0;
  }

  uint32_t layers() const override
  {
    return 1;
  }

  size_t size() const override
  {
    return this->image.sizeInBytes();
  }

  size_t size(size_t) const override
  {
    return this->image.sizeInBytes();
  }

  const unsigned char * data() const override
  {
    return this->image.bits();
  }

  VkFormat format() const override
  {
    return VK_FORMAT_R8G8B8A8_UNORM;
  }

  VkImageType image_type() const override
  {
    return VK_IMAGE_TYPE_2D;
  }

  VkImageViewType image_view_type() const override
  {
    return VK_IMAGE_VIEW_TYPE_2D;
  }

  QImage image;
};


class VulkanApplication : public QApplication {
public:
  VulkanApplication(int& argc, char** argv) :
    QApplication(argc, argv)
  {}

  bool notify(QObject* receiver, QEvent* event)
  {
    try {
      return QApplication::notify(receiver, event);
    }
    catch (std::exception & e) {
      std::cerr << std::string("caught exception in VulkanApplication::notify(): ") + typeid(e).name() << std::endl;
    }
    return false;
  }
};

class EventHandler : public QObject {
  Q_OBJECT
public:
  EventHandler(std::shared_ptr<VulkanInstance> vulkan,
               std::shared_ptr<VulkanDevice> device,
               std::shared_ptr<Group> root,
               std::shared_ptr<Camera> camera,
               QObject * parent = nullptr) : 
    QObject(parent),
    root(std::move(root)),
    camera(std::move(camera))
  {
    this->rendermanager = std::make_shared<RenderManager>(vulkan,
                                                          device,
                                                          VkExtent2D{ 512, 512 });

    this->rendermanager->init(this->root.get());
  }

protected:
  bool eventFilter(QObject *obj, QEvent *event) override
  {
    switch (event->type()) {
    case QEvent::Resize:
      QObject::eventFilter(obj, event);
      this->resizeEvent(static_cast<QResizeEvent*>(event));
      return true;
    case QEvent::KeyPress:
      this->keyPressEvent(static_cast<QKeyEvent*>(event));
      return true;
    case QEvent::MouseButtonPress:
      this->mousePressEvent(static_cast<QMouseEvent*>(event));
      return true;
    case QEvent::MouseButtonRelease:
      this->mouseReleaseEvent(static_cast<QMouseEvent*>(event));
      return true;
    case QEvent::MouseMove:
      this->mouseMoveEvent(static_cast<QMouseEvent*>(event));
      return true;
    default:
      // standard event processing
      return QObject::eventFilter(obj, event);
    }
  };

private:
  void resizeEvent(QResizeEvent * e)
  {
    VkExtent2D extent{ 
      static_cast<uint32_t>(e->size().width()),
      static_cast<uint32_t>(e->size().height())
    };
    this->rendermanager->resize(this->root.get(), extent);
  }

  void keyPressEvent(QKeyEvent * e)
  {
    switch (e->key()) {
    case Qt::Key_R:
      break;
    default:
      break;
    }
  }

  void mousePressEvent(QMouseEvent * e)
  {
    this->button = e->button();
    this->mouse_pos = { static_cast<float>(e->x()), static_cast<float>(e->y()) };
    this->mouse_pressed = true;
  }

  void mouseReleaseEvent(QMouseEvent *)
  {
    this->mouse_pressed = false;
  }

  void mouseMoveEvent(QMouseEvent * e)
  {
    if (this->mouse_pressed) {
      const vec2d pos = { static_cast<double>(e->x()), static_cast<double>(e->y()) };
      vec2d dx = (this->mouse_pos - pos) * .01;
      dx.v[1] = -dx.v[1];
      switch (this->button) {
      case Qt::MiddleButton: this->camera->pan(dx); break;
      case Qt::LeftButton: this->camera->orbit(dx); break;
      case Qt::RightButton: this->camera->zoom(dx.v[1]); break;
      default: break;
      }
      this->mouse_pos = pos;
      this->rendermanager->redraw(this->root.get());
    }
  }

public:
  std::shared_ptr<Group> root;
  std::shared_ptr<RenderManager> rendermanager;
  std::shared_ptr<Camera> camera;

  Qt::MouseButton button{ Qt::MouseButton::NoButton };
  bool mouse_pressed{ false };
  vec2d mouse_pos{};
};
