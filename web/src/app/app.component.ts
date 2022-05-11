import { Component, Inject, OnInit, OnDestroy } from '@angular/core';
import { ControllerService } from './controller.service';
import { Observable } from 'rxjs';
import { pluck } from 'rxjs/operators';

@Component({
    selector: 'app-root',
    templateUrl: './app.component.html',
    styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit, OnDestroy {
    baseUrl: string;
    title = 'Aquarium';
    //temperatures$: Observable<number>;
    boardInfo$: Observable<object>;
    constructor(@Inject(ControllerService) public controller: ControllerService) {
        //this.temperatures$ = this.controller.status$.pipe(pluck('CurrentTemperatures'));
        this.boardInfo$ = this.controller.boardInfo$;
    }
    ngOnInit() {
        const protocol = window.location.protocol.replace('http', 'ws');
        const host = window.location.host;
        this.baseUrl = `${protocol}//${host}/aquarium`;
        //this.baseUrl = "ws://aquarium:81";
        this.Connect();
    }
    ngOnDestroy() {
    }
    Connect() {
        this.controller.connect(this.baseUrl);
    }
    Reboot() {
        this.controller.Send({ Reboot: true });
    }
    BoardInfo() {
        this.controller.Send({ BoardInfo: true });
    }
    GetTemperatures(){
        this.controller.Send({ GetTemperatures: {Start: "", End: ""} });
    }
}