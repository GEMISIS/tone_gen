#include <gui/modules/menu.h>
#include <gui/modules/popup.h>

#include "tone_gen.h"
#include "app_context.h"

/** custom event handler - passes the event to the scene manager */
bool viewDispatcherCustomCallback(void* context, uint32_t custom_event) {
    furi_assert(context);
    struct AppContext_t* appContext = context;
    return scene_manager_handle_custom_event(appContext->scene_manager, custom_event);
}

/** navigation event handler - passes the event to the scene manager */
bool viewDispatcherNavigationCallback(void* context) {
    furi_assert(context);
    struct AppContext_t* appContext = context;
    return scene_manager_handle_back_event(appContext->scene_manager);
}

AppContextStatus initializeAppContext(
    struct AppContext_t** context,
    const SceneManagerHandlers* sceneManagerHandlers) {
    FURI_LOG_I(TAG, "Allocating memory for app context");

    *context = malloc(sizeof(struct AppContext_t));
    if(*context == NULL) {
        FURI_LOG_E(TAG, "Failed to allocate memory for app context");
        return APP_CONTEXT_CANT_ALLOCATE;
    }

    // Allocate our scene manager with the handlers provided
    FURI_LOG_I(TAG, "Setting up the scene manager");
    (*context)->scene_manager = scene_manager_alloc(sceneManagerHandlers, *context);

    // Now setup our view dispatchers
    FURI_LOG_I(TAG, "Setting up the view dispatcher");
    (*context)->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue((*context)->view_dispatcher);

    FURI_LOG_I(TAG, "Setting view dispatcher callbacks");
    view_dispatcher_set_event_callback_context((*context)->view_dispatcher, (*context));
    FURI_LOG_I(TAG, "Setting view dispatcher custom event callbacks");
    view_dispatcher_set_custom_event_callback(
        (*context)->view_dispatcher, viewDispatcherCustomCallback);
    FURI_LOG_I(TAG, "Setting view dispatcher navigation event callbacks");
    view_dispatcher_set_navigation_event_callback(
        (*context)->view_dispatcher, viewDispatcherNavigationCallback);
    FURI_LOG_I(TAG, "Setting view dispatcher callbacks done");

    return APP_CONTEXT_OK;
}

AppContextStatus freeAppContextViews(struct AppContext_t** context) {
    FURI_LOG_I(TAG, "Freeing views");
    struct ListNode_t* root = (*context)->activeViews;
    while(root) {
        struct View_t* view = root->data;
        view_dispatcher_remove_view((*context)->view_dispatcher, view->viewId);

        switch(view->type) {
        case MENU:
            menu_free(view->viewData);
            break;
        case POPUP:
            popup_free(view->viewData);
            break;
        }
        root = root->next;
    }
    FURI_LOG_I(TAG, "Removing all views from list");
    LinkedListStatus result = removeAllNodes(&(*context)->activeViews);
    if(result != LIST_OK) {
        return APP_CONTEXT_UNKNOWN_ERROR;
    }
    return APP_CONTEXT_OK;
}

AppContextStatus freeAppContext(struct AppContext_t** context) {
    FURI_LOG_I(TAG, "Ensuring views are free'd");
    AppContextStatus result = freeAppContextViews(context);
    if(result != APP_CONTEXT_OK) {
        return APP_CONTEXT_CANT_FREE_VIEWS;
    }
    FURI_LOG_I(TAG, "Freeing the scene");
    scene_manager_free((*context)->scene_manager);
    FURI_LOG_I(TAG, "Freeing the view dispatcher");
    view_dispatcher_free((*context)->view_dispatcher);
    FURI_LOG_I(TAG, "Freeing the app context");
    free((*context));
    return APP_CONTEXT_OK;
}