/* jshint expr: true */

define(["settings","./remotecontrol"],function(settings,rc) {
    "use strict";

    //an simple Update queue class that updates the server after a specified interval if the user does nothing else inbetween
    function UpdateQueue(url, finishedCallback) {
        if (!(this instanceof UpdateQueue))
            return new UpdateQueue(url,finishedCallback);

        this.url = url;
        this.isQueued = false;
        this.timer = undefined;
        this.finishedCallback = finishedCallback;
        this.lastData = undefined;
        this.lastSentData = undefined;
    }

    UpdateQueue.prototype.enqueue = function(data) {
        //what to do when enqueuing while sending?
        this.isQueued = true;
        this.lastData = data;

        this.timer && clearTimeout(this.timer);
        this.timer = setTimeout($.proxy(function() {
            this.isTransmitting = true;
            //post an update
            rc.postCmd(this.url, data, $.proxy(function(jqXHR, textStatus) {
                this.lastSentData = data;
                rc.forceUpdate();
                this.isQueued = false;
                if (this.finishedCallback) {
                    this.finishedCallback(jqXHR, textStatus);
                }
            }, this));
        }, this), settings.editUpdateDelay);
    };

    return UpdateQueue;
});