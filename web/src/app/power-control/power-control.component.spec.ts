import { ComponentFixture, TestBed } from '@angular/core/testing';

import { PowerControlComponent } from './power-control.component';

describe('PowerControlComponent', () => {
  let component: PowerControlComponent;
  let fixture: ComponentFixture<PowerControlComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ PowerControlComponent ]
    })
    .compileComponents();
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(PowerControlComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
