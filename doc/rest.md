# Plai REST API

## GET /_ping
Check whether the service is available
return 200

## PUT /media/\[image|video\]/\<name\>
Add a video or an image to the library.
Payload: binary data of the media.

## DELETE /media\[image|video\]/\<name\>
Delete a video or an image from the library.
The change might take effect later if the media is currently being played.

## POST /play?replay=true
Body: JSON with list of media paths to play.
Media paths have format `[image|video]/<name>`
Body:
```JSON
["video/media1", "image/media2", ...]
```

Parameters:
- `replay`: `true/false`
    - Wether to replay the medias from start after all are played. Default is true

