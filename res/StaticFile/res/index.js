
// 浏览器加载完成
window.onload = function () {

}



function ShowSettings() {
    console.log("ShowSettings");
    var settings = document.getElementById("MySettingBox");
    settings.style.visibility = "visible";
}

function GetPage(params) {
    var Box = document.getElementById("ChangeBox");
    Box.innerHTML = "";
    var data = JSON.parse(params);
    for (var i = 0; i < data.length; i++) {
        Box.innerHTML += "<a class=\"myitem\" href=" + data[i]['url'] + " target=\"_blank\">  \
                        <p><i class=\"fa fa-fw " + data[i]['fo'] + "\"></i>&nbsp;"+data[i]['title']+"</p>\
                    </a>"
        console.log(data[i]['url']);
        console.log(data[i]['title']);
        console.log(data[i]['fo']);
    }
}


function SendKey() {
    var key = document.getElementById("KeyInput").value;
    console.log("SendKey:" + key);
    var settings = document.getElementById("MySettingBox");
    settings.style.visibility = "hidden";

    //jquery方式  get请求
    $.ajax({
        type: "GET",
        url: "http://api.beiklive.top/HomePage",
        data: { "key": key },
        async: false,
        cache: false,
        success: function (backdata, status, xmlHttpRequest) {
            console.log("success");
            console.log(backdata);
            console.log(status);
            console.log(xmlHttpRequest);
            GetPage(backdata);
        }
    });
}