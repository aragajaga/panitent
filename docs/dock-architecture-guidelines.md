# Архитектурный Гайд Dock-системы (в стиле Visual Studio / AvalonDock)

Этот документ объясняет Dock-архитектуру с нуля: из каких частей она состоит, как эти части связаны и почему интерфейс ведёт себя предсказуемо.

## 1. Цель и UX-контракт

Правильная Dock-система должна гарантировать:

- в центре всегда `Workspace` (документы/редактор);
- док-панели живут в крайних зонах: `Left`, `Right`, `Top`, `Bottom`;
- панели внутри одной зоны работают как вкладки;
- drag/undock/dock не ломают состояние панели.

Если это не соблюдается, приложение быстро превращается в «случайное дерево сплиттеров».

## 2. Базовая модель

Используй модель **Shell + Zones + Panels**:

- `Root` -> корневой контейнер;
- `DockShell.*` -> только геометрия (сплиттеры);
- `DockZone.Left/Right/Top/Bottom` -> зоны докинга по краям;
- `Workspace` -> центральная рабочая область;
- `Panel` -> лист дерева с реальным `HWND`.

Ключевая идея: сплиттеры отвечают за форму лейаута, но не за смысловую модель панели.

## 3. Обязательные состояния зоны

У каждой боковой зоны должно быть явное состояние:

- `bCollapsed` — зона свернута или раскрыта;
- `hWndActiveTab` — активная вкладка в зоне.

Без этого вкладки начинают вести себя нестабильно.

## 4. Потоки работы

### 4.1 Инициализация

1. Создать `Workspace`.
2. Создать 4 зоны (`DockZone.*`).
3. Добавить стартовые панели в зоны.
4. Собрать shell вокруг `Workspace`.

### 4.2 Dock в зону

1. Определить сторону (`Left/Right/Top/Bottom`).
2. Найти соответствующую `DockZone.*`.
3. Добавить панель в стек зоны.
4. Сделать добавленную панель активной вкладкой.
5. Переложить layout (`Rearrange`).

### 4.3 Undock во floating

1. Взять исходный узел панели.
2. Удалить панель из текущей зоны/дерева.
3. Перепривязать `HWND` к floating-container.
4. Продолжить drag (без разрыва движения мышью).
5. Переложить layout хоста.

### 4.4 Поведение вкладок

1. Хит-тест по конкретной вкладке (сторона + `HWND` панели).
2. Клик по активной вкладке -> свернуть/развернуть всю зону.
3. Клик по неактивной -> сделать её активной и раскрыть зону.

## 5. Правила лейаута

- `Workspace` остаётся центральным якорем.
- `Workspace` — это core-панель: её нельзя откреплять или закрывать через dock-хром.
- Вкладки зоны раскладываются от начала полосы:
  - горизонтальные: слева направо;
  - вертикальные: сверху вниз.
- Полоса вкладок учитывает corner-inset, чтобы верх/низ и лево/право не конфликтовали в углах.
- Размеры grip должны быть ограничены (clamp).
- Свернутая зона не должна занимать место контента.
- Splitter-grip должен быть видимым, hit-testable и перетаскиваемым мышью.

## 6. Что не делать

- Не кодировать смысловую модель через случайные split-узлы.
- Не хранить активную вкладку «неявно» через порядок в дереве.
- Не дублировать формулы геометрии в нескольких файлах.
- Не смешивать бизнес-логику докинга и код отрисовки.

## 7. Стратегия тестирования

### 7.1 Обязательные unit-тесты

Покрыть `docklayout.*`:

- прямоугольники вкладок для всех сторон;
- проверка «стартового» расположения вкладок;
- clipping на маленьком клиентском прямоугольнике;
- clamp для stack/split grip;
- выбор ориентации стека по стороне.

### 7.2 Рекомендуемые интеграционные тесты

- dock в боковую зону активирует новую вкладку;
- сворачивание/разворачивание активной вкладки;
- клик по неактивной вкладке раскрывает зону и активирует её;
- undock сохраняет непрерывный drag.

## 8. Дисциплина изменений

При любом улучшении архитектуры:

1. Сначала обновляй тесты.
2. В том же коммите обновляй этот файл.
3. Добавляй запись в changelog ниже.

## 9. Changelog

- 2026-03-05:
  - добавлен общий модуль `docklayout.*` для геометрической политики Dock;
  - добавлен модуль `dockpolicy.*` для поведенческой политики Dock (core-панели, tab-click);
  - удалено дублирование формул зон из `dockhost.c` и `panitentapp.c`;
  - вкладки стандартизированы: от начала полосы, без центрирования;
  - добавлен live dock-preview overlay (подсветка side-target + центральные гайды) во время перемещения floating окна;
  - поправлено выравнивание текста на правых вертикальных вкладках;
  - добавлены интерактивные sizing grips для split-узлов в dock host;
  - добавлены unit-тесты `tests/docklayout_tests.c` для layout и behavior policy + запуск через `ctest`.

- 2026-03-05 (continued):
  - reduced dock resize flicker: WM_ERASEBKGND suppressed + backbuffer painting in dockhost.
  - added dock caption pin/unpin button that toggles owning zone collapsed state.
  - implemented VS-like auto-hide overlay for collapsed side zones: tab hover/click shows overlay without relayout;
  - clicking active auto-hide tab while overlay is shown repins the zone (collapsed=false).
  - auto-hide is click-only (no hover/timer open-close), with explicit close on outside click.
  - edge tabs are shown only for collapsed (auto-hide) zones; pinned zones do not render edge tabs.
  - adjusted auto-hide overlay frame drawing (separate content fill, caption band, border) and clickable pin on overlay caption.
  - root/tab gutter is now side-aware and appears only for sides that actually have collapsed tabs; no empty dock-site gaps.
  - click on auto-hide tab toggles overlay visibility only (show/hide) and never repins by itself.
  - auto-hide content now uses a dedicated overlay host window (wrapper), so controls render above pinned panes instead of behind root siblings.
  - auto-hide wrapper is now responsible for caption/chrome painting and caption button hit-test (pin/close), instead of drawing overlay chrome on root.
  - auto-hide caption button order differs from pinned windows: pin is drawn at the outer edge, close sits next to it.
  - auto-hide granularity moved to individual panels inside a dock side (no longer collapses the whole dock site at once).
  - caption/button chrome code for pinned, auto-hide and floating windows is unified through shared frame layout/draw/hit-test helpers to avoid divergent behavior.
  - added caption chevron button for docked, auto-hide and floating panels with a unified panel menu (`Doc&k`, `Auto Hide`, `Move To New Window`, `Close`).
  - menu item enable/disable policy now depends on panel state: `Doc&k` disabled for pinned, `Auto Hide` disabled for auto-hide/floating, `Move To New Window` disabled for floating.
  - caption glyph mapping aligned with VS-like states: pinned uses vertical pin tile `6`, auto-hide uses diagonal pin tile `1`, floating middle button uses maximize tile `3`.
  - edge auto-hide tabs now use variable length based on measured caption text instead of fixed tab length.
  - vertical edge-tab caption drawing is centered using text metrics (cross-axis and run-axis), improving visual alignment on both left and right sides.
