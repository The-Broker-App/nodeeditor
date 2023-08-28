#include "FlowView.hpp"

#include <QtWidgets/QGraphicsScene>

#include <QtGui/QPen>
#include <QtGui/QBrush>
#include <QtWidgets/QMenu>
#include <QSvgRenderer>
#include <QGraphicsSvgItem>

#include <QtCore/QRectF>
#include <QtCore/QPointF>

#include <QtOpenGL>
#include <QtWidgets>

#include <QDebug>
#include <iostream>
#include <cmath>

#include "FlowScene.hpp"
#include "DataModelRegistry.hpp"
#include "Node.hpp"
#include "NodeGraphicsObject.hpp"
#include "ConnectionGraphicsObject.hpp"
#include "StyleCollection.hpp"

using QtNodes::FlowView;
using QtNodes::FlowScene;

FlowView::
FlowView(QWidget *parent)
  : QGraphicsView(parent)
  , _clearSelectionAction(Q_NULLPTR)
  , _deleteSelectionAction(Q_NULLPTR)
  , _recentralizeSelectionAction(Q_NULLPTR)
  , _scene(Q_NULLPTR)
{
  setDragMode(QGraphicsView::ScrollHandDrag);
  setRenderHint(QPainter::Antialiasing);

  auto const &flowViewStyle = StyleCollection::flowViewStyle();

//  setBackgroundBrush(flowViewStyle.BackgroundColor);

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

//  setCacheMode(QGraphicsView::CacheBackground);
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

//  auto patternImage =  QPixmap(":/content/images/editor_pattern.svg");
//  _brushPattern = QBrush(patternImage);
//    _brushPattern = QBrush(QColor("#292B2B"));
//    _brushPattern.setStyle(Qt::Dense7Pattern);
//    setBackgroundBrush(_brushPattern);
//
//// 1. Crie um QPixmap
//    QPixmap pixmap(32, 32);
//    pixmap.fill(QColor("#292B2B")); // Preencha com a cor de fundo
//
//
//// 2. Desenhe o padrão no QPixmap
//    QPainter painter(&pixmap);
//    painter.setPen(QColor("#FFFFFF")); // Cor do padrão
//    for (int i = 0; i < pixmap.width(); i++) {
//        for (int j = 0; j < pixmap.height(); j++) {
//            // Este é um exemplo simples que simula o Dense7Pattern.
//            // Ajuste conforme necessário para se aproximar do padrão desejado.
//            if ((i + j) % 2 == 0) {
//                painter.drawPoint(i, j);
//            }
//        }
//    }
//    painter.end();
//
//// 3. Use o QPixmap como textura para o QBrush
//    QBrush brush(pixmap);
//
//// 4. Aplique o QBrush ao QGraphicsView
//    setBackgroundBrush(brush);
//    setRenderHint(QPainter::SmoothPixmapTransform);


//    QGraphicsSvgItem *item = new QGraphicsSvgItem(":/content/images/editor_pattern.svg");
//    addItem(item);

//  setBackgroundBrush(_brushPattern);


    //setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
}

void FlowView::dragEnterEvent(QDragEnterEvent *event){}
void FlowView::dropEvent(QDropEvent *event){}

FlowView::
FlowView(FlowScene *scene, QWidget *parent)
  : FlowView(parent)
{
  setScene(scene);
}


QAction*
FlowView::
clearSelectionAction() const
{
  return _clearSelectionAction;
}


QAction*
FlowView::
deleteSelectionAction() const
{
  return _deleteSelectionAction;
}

QAction* FlowView::recentralizeSelectionAction() const {
    return _recentralizeSelectionAction;
}

void
FlowView::setScene(FlowScene *scene)
{
  _scene = scene;
  QGraphicsView::setScene(_scene);

//    QGraphicsSvgItem *item = new QGraphicsSvgItem(":/content/images/editor_pattern.svg");
//    _scene->addItem(item);

  // setup actions
  delete _clearSelectionAction;
  _clearSelectionAction = new QAction(QStringLiteral("Clear Selection"), this);
  _clearSelectionAction->setShortcut(Qt::Key_Escape);
  connect(_clearSelectionAction, &QAction::triggered, _scene, &QGraphicsScene::clearSelection);
  addAction(_clearSelectionAction);

  delete _deleteSelectionAction;
  _deleteSelectionAction = new QAction(QStringLiteral("Delete Selection"), this);
//  _deleteSelectionAction->setShortcut(Qt::Key_Delete);
  connect(_deleteSelectionAction, &QAction::triggered, this, &FlowView::deleteSelectedNodes);
  addAction(_deleteSelectionAction);

  delete _recentralizeSelectionAction;
  _recentralizeSelectionAction = new QAction(QStringLiteral("Recentralize Selection"), this);
  connect(_recentralizeSelectionAction, &QAction::triggered, this, &FlowView::recentralize);
  addAction(_recentralizeSelectionAction);
}


void
FlowView::
contextMenuEvent(QContextMenuEvent *event)
{
    return;

  if (itemAt(event->pos()))
  {
    QGraphicsView::contextMenuEvent(event);
    return;
  }

  QMenu modelMenu;

  auto skipText = QStringLiteral("skip me");

  //Add filterbox to the context menu
  auto *txtBox = new QLineEdit(&modelMenu);

  txtBox->setPlaceholderText(QStringLiteral("Filter"));
  txtBox->setClearButtonEnabled(true);

  auto *txtBoxAction = new QWidgetAction(&modelMenu);
  txtBoxAction->setDefaultWidget(txtBox);

  modelMenu.addAction(txtBoxAction);

  //Add result treeview to the context menu
  auto *treeView = new QTreeWidget(&modelMenu);
  treeView->header()->close();

  auto *treeViewAction = new QWidgetAction(&modelMenu);
  treeViewAction->setDefaultWidget(treeView);

  modelMenu.addAction(treeViewAction);

  QMap<QString, QTreeWidgetItem*> topLevelItems;
  for (auto const &cat : _scene->registry().categories())
  {
    auto item = new QTreeWidgetItem(treeView);
    item->setText(0, cat);
    item->setData(0, Qt::UserRole, skipText);
    topLevelItems[cat] = item;
  }

  for (auto const &assoc : _scene->registry().registeredModelsCategoryAssociation())
  {
    auto parent = topLevelItems[assoc.second];
    auto item   = new QTreeWidgetItem(parent);
    item->setText(0, assoc.first);
    item->setData(0, Qt::UserRole, assoc.first);
  }

  treeView->expandAll();

  connect(treeView, &QTreeWidget::itemClicked, [&](QTreeWidgetItem *item, int)
  {
    QString modelName = item->data(0, Qt::UserRole).toString();

    if (modelName == skipText)
    {
      return;
    }

    auto type = _scene->registry().create(modelName);

    if (type)
    {
      auto& node = _scene->createNode(std::move(type));

      QPoint pos = event->pos();

      QPointF posView = this->mapToScene(pos);

      node.nodeGraphicsObject().setPos(posView);

      _scene->nodePlaced(node);
    }
    else
    {
      qDebug() << "Model not found";
    }

    modelMenu.close();
  });

  //Setup filtering
  connect(txtBox, &QLineEdit::textChanged, [&](const QString &text)
  {
    for (auto& topLvlItem : topLevelItems)
    {
      for (int i = 0; i < topLvlItem->childCount(); ++i)
      {
        auto child = topLvlItem->child(i);
        auto modelName = child->data(0, Qt::UserRole).toString();
        const bool match = (modelName.contains(text, Qt::CaseInsensitive));
        child->setHidden(!match);
      }
    }
  });

  // make sure the text box gets focus so the user doesn't have to click on it
  txtBox->setFocus();

  modelMenu.exec(event->globalPos());
}


void
FlowView::
wheelEvent(QWheelEvent *event)
{
  QPoint delta = event->angleDelta();

  if (delta.y() == 0)
  {
    event->ignore();
    return;
  }

  double const d = delta.y() / std::abs(delta.y());

  if (d > 0.0)
    scaleUp();
  else
    scaleDown();
}


void
FlowView::
scaleUp()
{
  double const step   = 1.2;
  double const factor = std::pow(step, 1.0);

  QTransform t = transform();

  if (t.m11() > 2.0)
    return;

  scale(factor, factor);
}


void
FlowView::
scaleDown()
{
    QTransform t = transform();

    if (t.m11() < 0.1)
        return;

  double const step   = 1.2;
  double const factor = std::pow(step, -1.0);

  scale(factor, factor);
}


void
FlowView::
deleteSelectedNodes()
{
  // Delete the selected connections first, ensuring that they won't be
  // automatically deleted when selected nodes are deleted (deleting a node
  // deletes some connections as well)
  for (QGraphicsItem * item : _scene->selectedItems())
  {
    if (auto c = qgraphicsitem_cast<ConnectionGraphicsObject*>(item))
      _scene->deleteConnection(c->connection());
  }

  // Delete the nodes; this will delete many of the connections.
  // Selected connections were already deleted prior to this loop, otherwise
  // qgraphicsitem_cast<NodeGraphicsObject*>(item) could be a use-after-free
  // when a selected connection is deleted by deleting the node.
  for (QGraphicsItem * item : _scene->selectedItems())
  {
    if (auto n = qgraphicsitem_cast<NodeGraphicsObject*>(item))
      _scene->removeNode(n->node());
  }
}

void FlowView::recentralize() {
    QPointF total(0, 0);
    for (QGraphicsItem * item : _scene->items()) {
        total += item->pos();
    }

    if (!_scene->items().empty())
        total /= _scene->items().size();

    resetTransform();

    auto newRect = sceneRect();
    newRect.moveTo(-total.x(), total.y());
    setSceneRect(newRect);
}

void
FlowView::
keyPressEvent(QKeyEvent *event)
{
  switch (event->key())
  {
    case Qt::Key_Shift:
      setDragMode(QGraphicsView::RubberBandDrag);
      break;

    default:
      break;
  }

  QGraphicsView::keyPressEvent(event);
}


void
FlowView::
keyReleaseEvent(QKeyEvent *event)
{
  switch (event->key())
  {
    case Qt::Key_Shift:
      setDragMode(QGraphicsView::ScrollHandDrag);
      break;

    default:
      break;
  }
  QGraphicsView::keyReleaseEvent(event);
}


void
FlowView::
mousePressEvent(QMouseEvent *event)
{
  QGraphicsView::mousePressEvent(event);
  if (event->button() == Qt::LeftButton)
  {
    _clickPos = mapToScene(event->pos());
  }
}


void
FlowView::
mouseMoveEvent(QMouseEvent *event)
{
  QGraphicsView::mouseMoveEvent(event);
  if (scene()->mouseGrabberItem() == nullptr && event->buttons() == Qt::LeftButton)
  {
    // Make sure shift is not being pressed
    if ((event->modifiers() & Qt::ShiftModifier) == 0)
    {
      QPointF difference = _clickPos - mapToScene(event->pos());
      setSceneRect(sceneRect().translated(difference.x(), difference.y()));
    }
  }
}

void
FlowView::
drawBackground(QPainter* painter, const QRectF& r)
{

    setRenderHint(QPainter::Antialiasing, true);
//    Cor de fundo
    painter->fillRect(r, QColor(25, 27, 27));

    // Salve a transformação atual
    QTransform savedTransform = painter->transform();

    // Desative a transformação enquanto desenha as cruzes
    painter->setTransform(QTransform());

    // Defina a cor da caneta
    painter->setPen(QColor(37, 39, 39)); // Cor do padrão

    qreal spacing = 32;  // Espaçamento entre cruzes

    // Calcula o retângulo visível baseado na viewport
    QRectF visibleArea = painter->viewport();

    int startX = static_cast<int>(visibleArea.left() - std::fmod(visibleArea.left(), spacing));
    int startY = static_cast<int>(visibleArea.top() - std::fmod(visibleArea.top(), spacing));

    qreal factor = transform().m11();
    qreal crossSize = 10.0*factor;

    for (int x = startX; x < visibleArea.right(); x += spacing) {
        for (int y = startY; y < visibleArea.bottom(); y += spacing) {
            painter->drawLine(QPointF(x-crossSize/2, y), QPointF(x+crossSize/2, y));  // linha horizontal da cruz
            painter->drawLine(QPointF(x, y-crossSize/2), QPointF(x, y+crossSize/2));  // linha vertical da cruz
        }
    }

    // Restaure a transformação original do painter
    painter->setTransform(savedTransform);

    return;
//    setRenderHint(QPainter::Antialiasing);
//    QGraphicsView::drawBackground(painter, r);
//    painter->fillRect(r,_brushPattern);

//     Cor de fundo
//    painter->fillRect(r, QColor(25, 27, 27));
//
//    // Define o padrão
//    painter->setPen(QColor(37, 39, 39)); // Cor do padrão
//
//
//    // Ajustar o espaçamento com base no zoom, se desejar
//    qreal factor = transform().m11();
//    qreal spacing = 32; // Espaçamento para o padrão
//
//    spacing /= factor;
//
//    painter->save();  // Salva o estado atual do painter
//
////    QTransform savedTransform = painter->transform();
////    painter->setTransform(QTransform());
//
//
//    for (qreal x = r.left(); x < r.right(); x += spacing) {
//        for (qreal y = r.top(); y < r.bottom(); y += spacing) {
//            painter->drawPoint(QPointF(x + 1, y));
//            painter->drawPoint(QPointF(x + 2, y));
//            painter->drawPoint(QPointF(x - 1, y));
//            painter->drawPoint(QPointF(x - 2, y));
//            painter->drawPoint(QPointF(x, y + 1));
//            painter->drawPoint(QPointF(x, y + 2));
//            painter->drawPoint(QPointF(x, y - 1));
//            painter->drawPoint(QPointF(x, y - 2));
//            painter->drawPoint(QPointF(x, y));
//        }
//    }
//
//    painter->restore();  // Restaura o estado anterior do painter


    return;
//  auto drawGrid =
//    [&](double gridStep)
//    {
//      QRect   windowRect = rect();
//      QPointF tl = mapToScene(windowRect.topLeft());
//      QPointF br = mapToScene(windowRect.bottomRight());
//
//      double left   = std::floor(tl.x() / gridStep - 0.5);
//      double right  = std::floor(br.x() / gridStep + 1.0);
//      double bottom = std::floor(tl.y() / gridStep - 0.5);
//      double top    = std::floor (br.y() / gridStep + 1.0);
//
//      //draw dots
////        for (int xi = int(left); xi <= int(right); ++xi)
////        {
////            for (int yi = int(bottom); yi <= int(top); ++yi)
////            {
////              QLineF line(xi * gridStep, (bottom + yi) * gridStep ,
////                          xi * gridStep, (top + yi) * gridStep) ;
////              qreal radius = 2.0f;
////              painter->drawEllipse(line.p1(),radius,radius);
////              painter->drawEllipse(line.p2(),radius,radius);
////            }
////        }
//
//      // vertical lines
////      for (int xi = int(left); xi <= int(right); ++xi)
////      {
////        QLineF line(xi * gridStep, bottom * gridStep,
////                    xi * gridStep, top * gridStep );
////
////        painter->drawLine(line);
////      }
//
//      // horizontal lines
////      for (int yi = int(bottom); yi <= int(top); ++yi)
////      {
////        QLineF line(left * gridStep, yi * gridStep,
////                    right * gridStep, yi * gridStep );
////        painter->drawLine(line);
////      }
//    };
//
//  auto const &flowViewStyle = StyleCollection::flowViewStyle();
//
//  QBrush bBrush = backgroundBrush();
//
//  QPen pfine(flowViewStyle.FineGridColor, 1.0);
//
//  painter->setPen(pfine);
//  drawGrid(15);
//
//  QPen p(flowViewStyle.CoarseGridColor, 1.0);
//
//  painter->setPen(p);
//  drawGrid(150);
}


void
FlowView::
showEvent(QShowEvent *event)
{
  _scene->setSceneRect(this->rect());
  QGraphicsView::showEvent(event);
}


FlowScene *
FlowView::
scene()
{
  return _scene;
}
